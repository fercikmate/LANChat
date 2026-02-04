#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <stdio.h>	
#include <string.h>
#include <winsock2.h>
#include "TCPfsm.h"

#define CLIENT_ADDRESS "255.255.255.255"
#define SERVER_PORT 8080
#define TCP_PORT 8081  // Separate port for TCP connections

extern char username[BUFFER_SIZE];

TCPComs::TCPComs() : FiniteStateMachine(TCP_FSM, TCP_MB, 10, 10, 10),
	m_tcpSocket(INVALID_SOCKET),
	ListenerThread(NULL),
	dwListenerThreadID(0),
	m_isServer(false),
	hConnectionThread(NULL),
	dwConnectionThreadID(0) {
	m_peerIP[0] = '\0';
	m_peerUsername[0] = '\0';
}

TCPComs::~TCPComs() {
}


uint8 TCPComs::GetAutomate() {
	return TCP_FSM;
}

/* This function actually connnects the AutoExamplee with the mailbox. */
uint8 TCPComs::GetMbxId() {
	return TCP_MB;
}

MessageInterface* TCPComs::GetMessageInterface(uint32 id) {
	if (id == 0)
		return &StandardMsgCoding;
	throw TErrorObject(__LINE__, __FILE__, 0x01010400);
}

void TCPComs::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void TCPComs::SetDefaultFSMData() {

}

void TCPComs::NoFreeInstances() {
	printf("[TCP_FSM] TCPComs::NoFreeInstances()\n");
}

void TCPComs::Initialize() {
	SetState(IDLE);

	// 2. Register the handler for the wake-up message sent from createConnectionInstance
	InitEventProc(IDLE, MSG_TCP_THREAD_START, (PROC_FUN_PTR)&TCPComs::ProcessConnectionRequest);

}
void TCPComs::Start() {
	printf("[TCP_FSM] TCPComs::Start() called\n");
}
void TCPComs::Connecting() {
	printf("[TCP_FSM] TCPComs::Connecting() called\n");
}

void TCPComs::ProcessConnectionRequest() {
	printf("[TCP_FSM] Processing request. Spawning worker thread...\n");

	// 1. Extract params from the message
	uint8* userParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);
	uint8* serverParam = GetParam(IS_SERVER_PARAM);

	char pName[BUFFER_SIZE];
	char pIP[16];

	// Offset 4 is typical for this kernel's parameter data alignment
	memcpy(pName, userParam + 4, userParam[2]);
	pName[userParam[2]] = '\0';

	memcpy(pIP, ipParam + 4, ipParam[2]);
	pIP[ipParam[2]] = '\0';

	bool isServer = (serverParam[4] == 1);

	// 2. Configure this specific instance
	SetPeerInfo(pIP, pName, isServer);

	// 3. SPAWN THE THREAD!
	// This gives you a different thread so the kernel can continue processing.
	hConnectionThread = ::CreateThread(
		NULL, 0,
		ConnectionWorkerThread,
		this, // Pass 'this' so the thread can access class members
		0, &dwConnectionThreadID
	);

	// 4. Return IMMEDIATELY so the Kernel can handle other messages!
}
DWORD WINAPI TCPComs::ConnectionWorkerThread(LPVOID param) {
	TCPComs* pThis = (TCPComs*)param; //pointer to our object

	printf("[TCP_Thread] Worker started for %s.\n", pThis->m_peerUsername);

	// Create the TCP socket first
	pThis->m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pThis->m_tcpSocket == INVALID_SOCKET) {
		printf("[TCP_Thread] socket creation failed: %d\n", WSAGetLastError());
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(TCP_PORT);  
	addr.sin_addr.s_addr = inet_addr(pThis->m_peerIP);

	// If Server, Listen. If Client, Connect
	if (pThis->m_isServer) {
		printf("[TCP_Thread] Setting up server for %s at %s...\n",
			pThis->m_peerUsername, pThis->m_peerIP);

		// Bind to any address on the TCP port
		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(TCP_PORT);  
		serverAddr.sin_addr.s_addr = INADDR_ANY;

		int reuse = 1;
		setsockopt(pThis->m_tcpSocket, SOL_SOCKET, SO_REUSEADDR,
			(char*)&reuse, sizeof(reuse));

		if (bind(pThis->m_tcpSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
			printf("[TCP_Thread] bind failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}

		if (listen(pThis->m_tcpSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("[TCP_Thread] listen failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}

		printf("[TCP_Thread] Listening for incoming connections for %s on port %d...\n", pThis->m_peerUsername, TCP_PORT);

		SOCKET clientSock = accept(pThis->m_tcpSocket, NULL, NULL);
		if (clientSock == INVALID_SOCKET) {
			printf("[TCP_Thread] accept failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}
		
		printf("[TCP_Thread] Accepted connection from %s for %s!\n", pThis->m_peerIP, pThis->m_peerUsername);
		// Store the client socket for communication
		closesocket(pThis->m_tcpSocket); // Close listener
		pThis->m_tcpSocket = clientSock; // Use client socket for communication
	}
	else {
		// Connect logic
		printf("[TCP_Thread] Connecting to server %s at %s:%d...\n",
		pThis->m_peerUsername, pThis->m_peerIP, TCP_PORT);
		

		if (connect(pThis->m_tcpSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
			printf("[TCP_Thread] connect failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}
		
		printf("[TCP_Thread] Connected to server %s!\n", pThis->m_peerUsername);
	}

	// Update state safely or just start the send/recv loop here.
	pThis->SetState(CONNECTED);

	return 0;
}

void TCPComs::ConnectionClosed() {
	// Cleanup...
	SetState(IDLE);
	FreeFSM();  // Return to free pool
}