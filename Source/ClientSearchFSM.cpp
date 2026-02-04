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
	printf("[%d] DeviceSearch::NoFreeInstances()\n", GetObjectId());
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
	printf("[%d] DeviceSearch::Start() called\n", GetObjectId());
	
	// LOGIN - Get username from user
	printf("===========================\n");
	printf("Welcome! Enter your username: \n");
	printf("===========================\n");

	if (fgets(username, BUFFER_SIZE, stdin) == NULL) {
		printf("Error reading username\n");
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
		printf("Username cannot be empty\n");
		return;
	}

	printf("Username set to: %s\n", username);
	
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

	printf("[%d] Broadcast ALIVE sent with username: %s\n", GetObjectId(),username);
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
		printf("Failed to create UDP socket: %d\n", WSAGetLastError());
		return;
	}

	int reuse = 1;
	setsockopt(m_udpSocket, SOL_SOCKET, SO_REUSEADDR,
		(char*)&reuse, sizeof(reuse));

	if (bind(m_udpSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		printf("Failed to bind UDP socket: %d\n", WSAGetLastError());
		closesocket(m_udpSocket);
		return;
	}

	
	printf("[%d] UDP listener started on port %d\n", GetObjectId(), SERVER_PORT);

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

	printf("[Thread] UDP listener thread started\n");

	while (true) {  
		memset(buffer, 0, BUFFER_SIZE);
		memset(&senderAddr, 0, sizeof(senderAddr));

		// Receive UDP message - leave room for null terminator
		int recvLen = recvfrom(pParent->m_udpSocket, buffer, BUFFER_SIZE - 1, 0,
			(struct sockaddr*)&senderAddr, &senderAddrLen);

		if (recvLen < 0) {
			printf("[Thread] recvfrom failed: %d\n", WSAGetLastError());
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

	printf("[Thread] UDP listener thread stopped\n");
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
			printf("[FSM] Invalid OK message from %s: %s\n", peerIP, data);
			return;
		}
		
		size_t senderLen = static_cast<size_t>(separator - payload);
		if (senderLen >= BUFFER_SIZE) {
			printf("[FSM] OK sender name too long from %s\n", peerIP);
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
		// Assume it's an ALIVE message with just the username
		msgType = MSG_UDP_ALIVE_RECEIVED;
		strcpy(content, data);
		printf("[FSM] Detected ALIVE message from %s, username: %s\n", peerIP, content);
	}

	if (strcmp(content, username) == 0) {
		//printf("[FSM] Ignoring own message from %s\n", peerIP);
		return;
	}

	// THREAD-SAFE: Use critical section or call handler directly
	// For some reason, posting messages to FSM from another thread causes issues.
	// Call the handler directly since we're on the listener thread
	if (msgType == MSG_UDP_ALIVE_RECEIVED) {
		// Call SendOk directly with the peer info
		SendOkDirect(content, peerIP);
	}
	else if (msgType == MSG_UDP_OK_RECEIVED) {
		// Call GotOk directly with the peer info
		GotOkDirect(content, peerIP);
	}
}
void DeviceSearch::SendOk() {
	// Send UDP OK response, create a new thread for a new fsm instance (each connection will have a new thread)
	//then wait for syn, send ack, etc.
	printf("[%d] SendOk called\n", GetObjectId());
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
		printf("[%d] Failed to create socket for OK response\n", GetObjectId());
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
		printf("[%d] Failed to send OK: %d\n", GetObjectId(), WSAGetLastError());
	}
	else {
		printf("[%d] OK sent to %s (%s)\n", GetObjectId(), peerUsername, peerIP);
	}

	closesocket(udpSocket);
}
void DeviceSearch::GotOk() {
	// Got OK respons, create a new thread for a new fsm instance (each connection will have a new thread)
	//then send a syn, wait for ack, etc.
	printf("[%d] GotOk called\n", GetObjectId());
	uint8* usernameParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);

	char peerUsername[BUFFER_SIZE];
	char peerIP[16];

	memcpy(peerUsername, usernameParam + 4, usernameParam[2]);
	peerUsername[usernameParam[2]] = '\0';

	memcpy(peerIP, ipParam + 4, ipParam[2]);
	peerIP[ipParam[2]] = '\0';

	printf("[%d] Received OK response from %s (%s)\n", GetObjectId(), peerUsername, peerIP);
	createConnectionInstance(peerIP, peerUsername, 1);
}

//FSMSystem TCP_connection_sys(10 /* max number of automates types */, 1 /* max number of msg boxes */);
DWORD WINAPI TCPConnectionThread(void* data) {
	TCPComs* pAutomate = new TCPComs();
	
	printf("[TCP Thread] Adding TCP automate to existing system...\n");
	
	// Add automate to an existing slot (uses the other Add overload)
	Client_sys.Add(pAutomate, TCP_FSM);  // No numOfObjects parameter!
	
	// Start automate
	pAutomate->Start();
	
	return 0;
}
void DeviceSearch::createConnectionInstance(const char* peerIP, const char* peerUsername, bool server) {
	printf("[%d] Creating TCP connection for %s (%s)\n",
		GetObjectId(), peerUsername, peerIP);

	// Create new TCP FSM on heap
	TCPComs* pTcpFsm = new TCPComs();
	pTcpFsm->SetPeerInfo(peerIP, peerUsername, server);

	// Add to system (system will manage it) - use the overload for adding to existing type
	Client_sys.Add(pTcpFsm, TCP_FSM);

	// Start the FSM (Initialize is called by Add)
	pTcpFsm->Start();


}

void DeviceSearch::SendOkDirect(const char* peerUsername, const char* peerIP) {
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
	broadcastAddr.sin_port = htons(SERVER_PORT);

	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		printf("[%d] Failed to create socket for OK response\n", GetObjectId());
		return;
	}

	int broadcast = 1;
	setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast));

	char okMessage[BUFFER_SIZE];
	sprintf(okMessage, "OK|%s|%s", username, peerUsername);

	int sendResult = sendto(udpSocket, okMessage, strlen(okMessage), 0,
		(struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr));

	if (sendResult == SOCKET_ERROR) {
		printf("[%d] Failed to send OK: %d\n", GetObjectId(), WSAGetLastError());
	}
	else {
		printf("[%d] OK sent to %s (%s)\n", GetObjectId(), peerUsername, peerIP);
	}

	closesocket(udpSocket);
}

void DeviceSearch::GotOkDirect(const char* peerUsername, const char* peerIP) {
	printf("[%d] Received OK response from %s (%s)\n", GetObjectId(), peerUsername, peerIP);
	createConnectionInstance(peerIP, peerUsername, true);
}