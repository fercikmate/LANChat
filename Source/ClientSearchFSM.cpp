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

// Make UDP_sys accessible (extern or pass as parameter)
extern FSMSystem Client_sys;

DeviceSearch::DeviceSearch() : FiniteStateMachine(UDP_FSM, UDP_MB, 10, 10, 10) {

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
	
}

/* Initial system message */
void DeviceSearch::GetUsername() {	
	
}

void DeviceSearch::Start() {
	printf("[UDP_FSM] DeviceSearch::Start() called\n");
	
	// LOGIN - Get username from user
	printf("===========================\n");
	printf("Welcome! Enter your username: \n");
	printf("===========================\n");

	if (fgets(username, BUFFER_SIZE, stdin) == NULL) {
		printf("[UDP_FSM] Error reading username\n");
		return;
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
		return;
	}

	printf("[UDP_FSM] Username set to: %s\n", username);
	
	// Send UDP ALIVE message with username
	StartUDPListening();

	SendUdpBroadcast();

	SetState(IDLE);
}

void DeviceSearch::SendUdpBroadcast() {
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");
	broadcastAddr.sin_port = htons(SERVER_PORT);

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int broadcast = 1;
	setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast));

	sendto(udpSocket, username, strlen(username), 0,
		(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

	printf("[UDP_FSM] Broadcast ALIVE sent with username: %s\n",username);
	closesocket(udpSocket);
}

void DeviceSearch::StartUDPListening() {
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
	DeviceSearch* pParent = (DeviceSearch*)param; //param is 'this' DeviceSearch instance
	char buffer[BUFFER_SIZE];
	sockaddr_in senderAddr;
	int senderAddrLen = sizeof(senderAddr);

	printf("[UDP_Listener_Thread] UDP listener thread started\n");

	while (true) {  
		memset(buffer, 0, BUFFER_SIZE);
		memset(&senderAddr, 0, sizeof(senderAddr));

		// Receive UDP message - leave room for null terminator
		int recvLen = recvfrom(pParent->m_udpSocket, buffer, BUFFER_SIZE - 1, 0,
			(struct sockaddr*)&senderAddr, &senderAddrLen);

		if (recvLen < 0) {
			printf("[UDP_Listener_Thread] recvfrom failed: %d\n", WSAGetLastError());
			break;
		}

		if (recvLen > 0) {
			buffer[recvLen] = '\0';
			// Convert network message to FSM message
			pParent->UdpMsg_2_FSMMsg(buffer, recvLen, &senderAddr);
		}
		else {
			// No data received, continue listening
			Sleep(100);
		}
	}

	printf("[UDP_Listener_Thread] UDP listener thread stopped\n");
	return 0;
}
void DeviceSearch::UdpMsg_2_FSMMsg(const char* data, int length, sockaddr_in* sender) {
	char peerIP[16];
	strcpy(peerIP, inet_ntoa(sender->sin_addr));
	uint16 msgType;
	char content[BUFFER_SIZE];

	if (strncmp(data, "OK|", 3) == 0) {
		msgType = MSG_UDP_OK_RECEIVED;
		char senderName[BUFFER_SIZE];
		char targetName[BUFFER_SIZE];
		const char* payload = data + 3;
		const char* separator = strchr(payload, '|');

		if (separator == nullptr) {
			printf("[UDP_FSM] Invalid OK message from %s: %s\n", peerIP, data);
			return;
		}
		
		size_t senderLen = static_cast<size_t>(separator - payload);
		if (senderLen >= BUFFER_SIZE) {
			printf("[UDP_FSM] OK sender name too long from %s\n", peerIP);
			return;
		}
		// Extract sender and target names
		strncpy(senderName, payload, senderLen);
		senderName[senderLen] = '\0';
		strncpy(targetName, separator + 1, BUFFER_SIZE - 1);
		targetName[BUFFER_SIZE - 1] = '\0';

		if (strcmp(targetName, username) != 0) {
			//printf("[FSM] OK message not for us (%s), ignoring\n", targetName);
			return;
		}
		//Its for us->content
		strcpy(content, senderName);
	}
	else {
		//ignore own alive messages
		if (strcmp(data, username) == 0) {
			//printf("[FSM] Ignoring own ALIVE message from %s\n", peerIP);
			return;
		}
		// Assume it's an ALIVE message with just the username
		msgType = MSG_UDP_ALIVE_RECEIVED;
		strcpy(content, data);
		printf("[UDP_FSM] Detected ALIVE message from %s, username: %s\n", peerIP, content);
	}
	//ignore own messages
	if (strcmp(content, username) == 0) {	
		//printf("[FSM] Ignoring own message from %s\n", peerIP);
		return;
	}

	//UDP message is valid and not from us, prepare FSM message	
	PrepareNewMessage(0x00,msgType);
	SetMsgToAutomate(UDP_FSM);
	SetMsgObjectNumberTo(0);
	AddParam(PARAM_USERNAME, (uint16)strlen(content), (uint8*)content);
	AddParam(PARAM_IP_ADDRESS, (uint16)strlen(peerIP), (uint8*)peerIP);
	SendMessage(UDP_MB);
	
}
void DeviceSearch::SendOk() {
	// Send UDP OK response, create a new thread for a new fsm instance (each connection will have a new thread)
	//then wait for syn, send ack, etc.
	printf("[UDP_FSM] SendOk called\n");
	uint8* usernameParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);

	char peerUsername[BUFFER_SIZE];
	char peerIP[16];

	memcpy(peerUsername, usernameParam + 4, usernameParam[2]);
	peerUsername[usernameParam[2]] = '\0';

	memcpy(peerIP, ipParam + 4, ipParam[2]);
	peerIP[ipParam[2]] = '\0';

	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
	broadcastAddr.sin_port = htons(SERVER_PORT);

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		printf("[UDP_FSM] Failed to create socket for OK response\n");
		return;
	}

	// Format: "OK|username"
	int broadcast = 1;
	setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast));

	char okMessage[BUFFER_SIZE];
	sprintf(okMessage, "OK|%s|%s", username, peerUsername);

	int sendResult = sendto(udpSocket, okMessage, strlen(okMessage), 0,
		(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));
	//sleep a bit to avoid message clumping
	Sleep(100);
	if (sendResult == SOCKET_ERROR) {
		printf("[UDP_FSM] Failed to send OK: %d\n", WSAGetLastError());
	}
	else {
		printf("[UDP_FSM] OK sent to %s (%s)\n", peerUsername, peerIP);
	}

	closesocket(udpSocket);
}
void DeviceSearch::GotOk() {
	// Got OK response, create a new thread for a new fsm instance (each connection will have a new thread)
	//then send a syn, wait for ack, etc.
	printf("[UDP_FSM] GotOk called\n");
	uint8* usernameParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);

	char peerUsername[BUFFER_SIZE];
	char peerIP[16];

	memcpy(peerUsername, usernameParam + 4, usernameParam[2]);
	peerUsername[usernameParam[2]] = '\0';

	memcpy(peerIP, ipParam + 4, ipParam[2]);
	peerIP[ipParam[2]] = '\0';

	printf("[UDP_FSM] Received OK response from %s (%s)\n", peerUsername, peerIP);
	createConnectionInstance(peerIP, peerUsername, 1);
}



extern HANDLE hFsmMutex;
void DeviceSearch::createConnectionInstance(const char* peerIP, const char* peerUsername, bool server) {
	printf("[UDP_FSM] Requesting TCP connection for %s (%s)\n", peerUsername, peerIP);

	/* To be truly thread-safe, we send a message to the TCP_FSM mailbox.
	   The kernel will then deliver this to one of the TCPComs instances
	   that is currently in an 'IDLE' or 'INITIAL' state.
	*/

	//can use mutex for for building message from the UDP thread, if new client connections are frequent
	//WaitForSingleObject(hFsmMutex, INFINITE);

	PrepareNewMessage(0x00, MSG_TCP_THREAD_START); // Reuse your existing msg type or create a new one
	SetMsgToAutomate(TCP_FSM);
	SetMsgObjectNumberTo(0xffffffff);
	AddParam(PARAM_USERNAME, (uint16)strlen(peerUsername), (uint8*)peerUsername);
	AddParam(PARAM_IP_ADDRESS, (uint16)strlen(peerIP), (uint8*)peerIP);

	// You could add a param for 'server' status here
	uint8 isServer = server ? 1 : 0;
	//this param indicates if this automate is server or client
	//indicates if this automate is server or client1 for server, 0 for client
	AddParam(IS_SERVER_PARAM, 1, &isServer);
	SendMessage(TCP_MB);

	//ReleaseMutex(hFsmMutex);
}

