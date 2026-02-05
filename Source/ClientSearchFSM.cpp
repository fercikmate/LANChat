#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <stdio.h>	
#include <string.h>
#include <winsock2.h>
#include "ClientSearchFSM.h"
#include "../kernel/LogFile.h"
#include "TCPfsm.h"


#define CLIENT_ADDRESS "255.255.255.255"
#define SERVER_PORT 8080

extern char username[BUFFER_SIZE];

DeviceSearch::DeviceSearch() : FiniteStateMachine(UDP_FSM, UDP_MB, 10, 10, 10),
	m_udpSocket(INVALID_SOCKET),
	ListenerThread(NULL),
	ThreadID(0),
	m_retryCount(0),
	m_consoleThreadStarted(false) {

}

DeviceSearch::~DeviceSearch() {
}


uint8 DeviceSearch::GetAutomate() {
	return UDP_FSM;
}

/* This function actually connnects the AutoExamplee with the mailbox. */
uint8 DeviceSearch::GetMbxId() {
	return UDP_MB;
}

MessageInterface* DeviceSearch::GetMessageInterface(uint32 id) {
	if (id == 0)
		return &StandardMsgCoding;
	throw TErrorObject(__LINE__, __FILE__, 0x01010400);
}

void DeviceSearch::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void DeviceSearch::SetDefaultFSMData() {

}

void DeviceSearch::NoFreeInstances() {
	printf("[UDP_FSM] DeviceSearch::NoFreeInstances()\n");
}

void DeviceSearch::Initialize() {
	SetState(LOGIN);
	InitEventProc(IDLE, MSG_UDP_ALIVE_RECEIVED, (PROC_FUN_PTR)&DeviceSearch::SendOk);
	InitEventProc(IDLE, MSG_UDP_OK_RECEIVED, (PROC_FUN_PTR)&DeviceSearch::GotOk);
	InitEventProc(IDLE, TIMER1_EXPIRED, (PROC_FUN_PTR)&DeviceSearch::OnTimerExpired);
	InitTimerBlock(TIMER1_ID, TIMER1_COUNT, TIMER1_EXPIRED);
}


extern char username[BUFFER_SIZE];
void DeviceSearch::Start() {
	printf("[UDP_FSM] DeviceSearch::Start() called\n");
	if (getUsername() == -1) {
		printf("Press a button to exit the application due to invalid username.\n");
		char ch = _getch();
		return;
	}
	
	StartUDPListening();
	// Send UDP ALIVE message with username
	SendUdpBroadcast();
	
	SetState(IDLE);
}
extern int myTcpPort;
void DeviceSearch::SendUdpBroadcast() {
	//msg struct is "ALIVE|username|tcpPort"
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
	broadcastAddr.sin_port = htons(SERVER_PORT);

	

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int broadcast = 1;
	setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast));

	char payload[BUFFER_SIZE];
	sprintf(payload, "%s|%d", username, myTcpPort);

	sendto(udpSocket, payload, strlen(payload), 0,
		(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
	//safety stop
	StopTimer(TIMER1_ID);
	StartTimer(TIMER1_ID);


	printf("[UDP_FSM] Broadcast ALIVE sent with username: %s\n", payload);
	closesocket(udpSocket);
}

void DeviceSearch::StartUDPListening() {
	//create a thread to listen for UDP messages
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(SERVER_PORT);

	m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_udpSocket == INVALID_SOCKET) {
		printf("[UDP_FSM] Failed to create UDP socket: %d\n", WSAGetLastError());
		return;
	}

	int reuse = 1;
	setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR,
		(char*)&reuse, sizeof(reuse));

	if (bind(m_udpSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		printf("[UDP_FSM] Failed to bind UDP socket: %d\n", WSAGetLastError());
		closesocket(m_udpSocket);
		return;
	}

	
	printf("[UDP_FSM] UDP listener started on port %d\n", SERVER_PORT);

	// Create listener thread using Windows API, not member function
	ListenerThread = ::CreateThread(
		NULL, 0, UdpListenerThread,
		(LPVOID)this, 0, &ThreadID
	);
}

DWORD WINAPI DeviceSearch::UdpListenerThread(LPVOID param) {
	DeviceSearch* pParent = (DeviceSearch*)param;
	char buffer[BUFFER_SIZE];
	sockaddr_in senderAddr;
	int senderAddrLen = sizeof(senderAddr);

	printf("[UDP_Listener_Thread] UDP listener thread started\n");

	while (true) {  
		memset(buffer, 0, BUFFER_SIZE);
		memset(&senderAddr, 0, sizeof(senderAddr));

		int recvLen = recvfrom(pParent->m_udpSocket, buffer, BUFFER_SIZE - 1, 0,
			(struct sockaddr*)&senderAddr, &senderAddrLen);

		if (recvLen < 0) {
			printf("[UDP_Listener_Thread] recvfrom failed: %d\n", WSAGetLastError());
			break;
		}

		if (recvLen > 0) {
			buffer[recvLen] = '\0';
			
			// Stop timer if its ok message-> connecting soon
			if (strncmp(buffer, "OK|", 3) == 0) {
				pParent->StopTimer(TIMER1_ID);
			}
			
			// Convert network message to FSM message
			pParent->UdpMsg_2_FSMMsg(buffer, recvLen, &senderAddr);
		}
		else {
			Sleep(100);
		}
	}

	printf("[UDP_Listener_Thread] UDP listener thread stopped\n");
	return 0;
}
void DeviceSearch::UdpMsg_2_FSMMsg(const char* data, int length, sockaddr_in* sender) {
	char buffer[BUFFER_SIZE];
	// Ensure we don't overflow and null-terminate the incoming raw data
	int copyLen = (length < BUFFER_SIZE) ? length : BUFFER_SIZE - 1;
	memcpy(buffer, data, copyLen);
	buffer[copyLen] = '\0';

	char peerIP[16];
	strcpy(peerIP, inet_ntoa(sender->sin_addr));

	uint16 msgType;
	char peerUsername[BUFFER_SIZE];
	uint16 peerTcpPort = 8081; // fallback port, shouldnt have to be used

	// Check if it is an OK message: "OK|senderUsername|targetUsername|port"
	if (strncmp(buffer, "OK|", 3) == 0) {
		msgType = MSG_UDP_OK_RECEIVED;

		char* senderPart = buffer + 3;          // Move past "OK|"
		char* pipePtr = strchr(senderPart, '|'); // Find separator between sender and target
		// If no pipe found, invalid format, ignore
		if (pipePtr == nullptr) return;

		*pipePtr = '\0';
		char* targetPart = pipePtr + 1;

		char* pipe2 = strchr(targetPart, '|');
		if (pipe2 != nullptr) {
			*pipe2 = '\0'; // Null terminate targetUsername so strcmp works
			peerTcpPort = (uint16)atoi(pipe2 + 1);
		}
		// Verify this OK message was meant for us
		if (strcmp(targetPart, username) != 0) {
			return;
		}

		strcpy(peerUsername, senderPart);
		//get port 
		char* portPtr = strchr(targetPart, '|');
		if (portPtr != nullptr) {
			peerTcpPort = (uint16)atoi(portPtr + 1); // Convert port string to integer
		}
	}
		// Assume it's an ALIVE message: "username|port"
		else {
			msgType = MSG_UDP_ALIVE_RECEIVED;

			char* pipePtr = strchr(buffer, '|');
			if (pipePtr == nullptr) {
				// If no pipe is found, it's either old format or invalid
				return;
			}

			*pipePtr = '\0';
			strcpy(peerUsername, buffer);
			peerTcpPort = (uint16)atoi(pipePtr + 1); // Convert port string to integer
			// Ignore ALIVE messages from ourselves
			if (strcmp(peerUsername, username) == 0) return;
			
			printf("\n[UDP_FSM] Detected ALIVE from %s, User: %s, Port: %d\n", peerIP, peerUsername, peerTcpPort);
		}

		
		if (strcmp(peerUsername, username) == 0) return;


		//  Prepare and send the FSM message
		PrepareNewMessage(0x00, msgType);
		SetMsgToAutomate(UDP_FSM);
		SetMsgObjectNumberTo(0);

		AddParam(PARAM_USERNAME, (uint16)strlen(peerUsername), (uint8*)peerUsername);
		AddParam(PARAM_IP_ADDRESS, (uint16)strlen(peerIP), (uint8*)peerIP);

		AddParam(PARAM_PORT, 2, (uint8*)&peerTcpPort);

		SendMessage(UDP_MB);
	
}
void DeviceSearch::SendOk() {
	// Send UDP OK response, create a new thread for a new fsm instance (each connection will have a new thread)
	//then wait for syn, send ack, etc.
	printf("[UDP_FSM] SendOk called\n");
	uint8* usernameParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);
	uint8* portParam = GetParam(PARAM_PORT);
	uint16 actualPort = *(uint16*)(portParam + 4);

	char peerUsername[BUFFER_SIZE];
	char peerIP[16];

	memcpy(peerUsername, usernameParam + 4, usernameParam[2]);
	peerUsername[usernameParam[2]] = '\0';

	memcpy(peerIP, ipParam + 4, ipParam[2]);
	peerIP[ipParam[2]] = '\0';

	if (ConnectionExistsForPeer(peerUsername, peerIP)) {
		printf("[UDP_FSM] Connection to %s (%s) already exists, skipping.\n",
			peerUsername, peerIP);
		return;
	}
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
	broadcastAddr.sin_port = htons(SERVER_PORT);

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		printf("[UDP_FSM] Failed to create socket for OK response\n");
		return;
	}
	

	printf("[UDP_FSM] Requesting TCP connection for %s (%s)\n", peerUsername, peerIP);
	// Format: "OK|username|port"
	int broadcast = 1;
	setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast));

	char okMessage[BUFFER_SIZE];
	sprintf(okMessage, "OK|%s|%s|%d", username, peerUsername, myTcpPort);

	int sendResult = sendto(udpSocket, okMessage, strlen(okMessage), 0,
		(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
	//sleep a bit to avoid message clumping
	Sleep(100);
	if (sendResult == SOCKET_ERROR) {
		printf("[UDP_FSM] Failed to send OK: %d\n", WSAGetLastError());
	}
	else {
		printf("[UDP_FSM] OK sent to %s (%s)\n", peerUsername, peerIP);
		//check if console thread is already started
		if (!m_consoleThreadStarted) {
			m_consoleThreadStarted = true;
			::CreateThread(NULL, 0, ConsoleInputThread, (LPVOID)this, 0, NULL);
			printf("[UDP_FSM] Username confirmed, chat enabled.\n");
		}
		createConnectionInstance(peerIP, peerUsername, actualPort, 1);
	}

	closesocket(udpSocket);
}
void DeviceSearch::GotOk() {
	// Got OK response, create a new thread for a new fsm instance (each connection will have a new thread)
	//then send a syn, wait for ack, etc.
	printf("[UDP_FSM] GotOk called\n");
	uint8* usernameParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);
	uint8* portParam = GetParam(PARAM_PORT);
	uint16 actualPort = *(uint16*)(portParam + 4);

	char peerUsername[BUFFER_SIZE];
	char peerIP[16];

	memcpy(peerUsername, usernameParam + 4, usernameParam[2]);
	peerUsername[usernameParam[2]] = '\0';

	memcpy(peerIP, ipParam + 4, ipParam[2]);
	peerIP[ipParam[2]] = '\0';

	//check if console thread is already started
	if (!m_consoleThreadStarted) {
		m_consoleThreadStarted = true;
		::CreateThread(NULL, 0, ConsoleInputThread, (LPVOID)this, 0, NULL);
		printf("[UDP_FSM] Username confirmed, chat enabled.\n");
	}

	printf("[UDP_FSM] Received OK response from %s (%s)\n", peerUsername, peerIP);
	createConnectionInstance(peerIP, peerUsername, actualPort, 0);
}



extern HANDLE hFsmMutex;
void DeviceSearch::createConnectionInstance(const char* peerIP, const char* peerUsername,uint16 port, bool server) {
	printf("[UDP_FSM] Requesting TCP connection for %s (%s)\n", peerUsername, peerIP);

	/* send a message to the TCP_FSM mailbox.*/

	//can use mutex for for building message from the UDP thread, if new client connections are frequent
	//WaitForSingleObject(hFsmMutex, INFINITE);

	PrepareNewMessage(0x00, MSG_TCP_THREAD_START); // Reuse your existing msg type or create a new one
	SetMsgToAutomate(TCP_FSM);
	SetMsgObjectNumberTo(0xffffffff);
	AddParam(PARAM_USERNAME, (uint16)strlen(peerUsername), (uint8*)peerUsername);
	AddParam(PARAM_IP_ADDRESS, (uint16)strlen(peerIP), (uint8*)peerIP);
	AddParam(PARAM_PORT, 2, (uint8*)&port);

	// You could add a param for 'server' status here
	uint8 isServer = server ? 1 : 0;
	
	//indicates if this automate is server or client 1 for server, 0 for client
	AddParam(IS_SERVER_PARAM, 1, &isServer);
	SendMessage(TCP_MB);

	//ReleaseMutex(hFsmMutex);
}

extern TCPComs* tcpInstances; 
extern int tcpInstanceCount;

bool ConnectionExistsForPeer(const char* username, const char* ip) {
    // Iterate through all TCP FSM instances
    for (int i = 0; i < tcpInstanceCount; i++) {
        if (tcpInstances[i].IsConnectedToPeer()) {
            // Check if this instance is already connected to this peer
            if (strcmp(tcpInstances[i].GetPeerUsername(), username) == 0 &&
                strcmp(tcpInstances[i].GetPeerIP(), ip) == 0) {
                return true;  // Connection already exists
            }
        }
    }
    return false;
}
int DeviceSearch::getUsername() {
	

	// LOGIN - Get username from user
	printf("===========================\n");
	printf("Welcome! Enter your username: \n");
	printf("===========================\n");

	if (fgets(username, BUFFER_SIZE, stdin) == NULL) {
		printf("[UDP_FSM] Error reading username\n");
		return -1;
	}

	// Remove newline character if present
	size_t len = strlen(username);
	if (len > 0 && username[len - 1] == '\n') {
		username[len - 1] = '\0';
		len--;
	}

	// Check if username is empty	
	if (len == 0) {
		printf("[UDP_FSM] Username cannot be empty\n");
		return -1;
	}

	printf("[UDP_FSM] Username set to: %s\n", username);

	for (int i = 0; username[i] != '\0'; i++) myTcpPort += username[i];
	return 0;

}
DWORD WINAPI DeviceSearch::ConsoleInputThread(LPVOID param) {
	DeviceSearch* pThis = (DeviceSearch*)param;
	char inputBuffer[BUFFER_SIZE];
	while (true) {
		if (fgets(inputBuffer, BUFFER_SIZE, stdin)) {
			inputBuffer[strcspn(inputBuffer, "\n")] = 0;

			//mutex to safely build message from console thread
			WaitForSingleObject(hFsmMutex, INFINITE);

			for (int i = 0; i < 10; i++) {
				pThis->PrepareNewMessage(0x00, MSG_USER_INPUT);
				pThis->SetMsgToAutomate(TCP_FSM);
				pThis->SetMsgObjectNumberTo(i);
				pThis->AddParam(PARAM_DATA, (uint16)strlen(inputBuffer), (uint8*)inputBuffer);
				pThis->SendMessage(TCP_MB);
			}
			ReleaseMutex(hFsmMutex);
		}
	}
	return 0;
}
void DeviceSearch::OnTimerExpired() {
	m_retryCount++;

		// assume we're alone OR username is taken
		printf("[UDP_FSM] No response after %d seconds. Enter new username:\n", TIMER1_COUNT);
		m_retryCount = 0;

		if (getUsername() == -1) {
			printf("Invalid name.\n");
			return;
		}
		SendUdpBroadcast();
}
