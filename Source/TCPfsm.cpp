#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <stdio.h>	
#include <string.h>
#include <winsock2.h>
#include "TCPfsm.h"

#define CLIENT_ADDRESS "255.255.255.255"



extern char username[BUFFER_SIZE];
extern int myTcpPort;
TCPComs::TCPComs() : FiniteStateMachine(TCP_FSM, TCP_MB, 10, 10, 10),
	m_tcpSocket(INVALID_SOCKET),
	ListenerThread(NULL),
	dwListenerThreadID(0),
	m_isServer(false),
	m_peerPort(0),
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

	//  Register the handler for the wake-up message sent from createConnectionInstance
	InitEventProc(IDLE, MSG_TCP_THREAD_START, (PROC_FUN_PTR)&TCPComs::ProcessConnectionRequest);
	InitEventProc(CONNECTED, MSG_USER_INPUT, (PROC_FUN_PTR)&TCPComs::HandleSendData);
//	InitTimerBlock(TIMER2_ID, TIMER2_COUNT, TIMER2_EXPIRED);
//	InitEventProc(CONNECTED, TIMER2_EXPIRED, (PROC_FUN_PTR)&TCPComs::OnTimerExpired);
}
void TCPComs::Start() {
	printf("[TCP_FSM] TCPComs::Start() called\n");
}
void TCPComs::Connecting() {
	printf("[TCP_FSM] TCPComs::Connecting() called\n");
}

void TCPComs::ProcessConnectionRequest() {
	//printf("[TCP_FSM] Processing request. Spawning worker thread...\n");
	
	// Extract parameters from the message
	uint8* userParam = GetParam(PARAM_USERNAME);
	uint8* ipParam = GetParam(PARAM_IP_ADDRESS);
	uint8* serverParam = GetParam(IS_SERVER_PARAM);
	uint8* portParam = GetParam(PARAM_PORT);
	this->m_peerPort = *(uint16*)(portParam + 4);
	// printf("[TCP_FSM] Requested port: %d\n", this->m_peerPort);

	char pName[BUFFER_SIZE];
	char pIP[16];

	//4 bytes before actual data starts
	memcpy(pName, userParam + 4, userParam[2]);
	pName[userParam[2]] = '\0';

	memcpy(pIP, ipParam + 4, ipParam[2]);
	pIP[ipParam[2]] = '\0';

	bool isServer = (serverParam[4] == 1);

	//this specific instance
	SetPeerInfo(pIP, pName, isServer);

	// thread spawn
	hConnectionThread = ::CreateThread(
		NULL, 0,
		ConnectionWorkerThread,
		this, 
		0, &dwConnectionThreadID
	);

	
}
DWORD WINAPI TCPComs::ConnectionWorkerThread(LPVOID param) {
	TCPComs* pThis = (TCPComs*)param; //pointer to our object

	//printf("[TCP_Thread] Worker started for %s.\n", pThis->m_peerUsername);

	// Create the TCP socket first
	pThis->m_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (pThis->m_tcpSocket == INVALID_SOCKET) {
		printf("[TCP_Thread] socket creation failed: %d\n", WSAGetLastError());
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(pThis->m_peerPort);  
	addr.sin_addr.s_addr = inet_addr(pThis->m_peerIP);

	// If Server, Listen. If Client, Connect
	if (pThis->m_isServer) {
		/*printf("[TCP_Thread] Setting up server for %s at %s at port %d...\n",
			pThis->m_peerUsername, pThis->m_peerIP, pThis->m_peerPort);*/

		// Bind to any address on the TCP port
		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(myTcpPort);  
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
		Sleep(100); // Sleep a bit to ensure the port is properly released if recently used

		if (listen(pThis->m_tcpSocket, SOMAXCONN) == SOCKET_ERROR) {
			printf("[TCP_Thread] listen failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}

		printf("[TCP_Thread] Listening for incoming connections for %s on port %d...\n", pThis->m_peerUsername, myTcpPort);

		SOCKET clientSock = accept(pThis->m_tcpSocket, NULL, NULL);
		if (clientSock == INVALID_SOCKET) {
			printf("[TCP_Thread] accept failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}
		
		printf("[TCP_Thread] Accepted connection from %s:%d for user %s!\n\n", pThis->m_peerIP,pThis->m_peerPort, pThis->m_peerUsername);
		// Store the client socket for communication
		closesocket(pThis->m_tcpSocket); // Close listener
		pThis->m_tcpSocket = clientSock; // Use client socket for communication
	}

	//CLIENT LOGIC
	else {
		//if we're a client, we need to connect to the server

		printf("[TCP_Thread] Connecting to user %s at %s:%d...\n\n",
		pThis->m_peerUsername, pThis->m_peerIP, pThis->m_peerPort);
		

		int retries = 0;
		const int MAX_RETRIES = 5;
		
		while (retries < MAX_RETRIES) {
			if (connect(pThis->m_tcpSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
				int err = WSAGetLastError();
				if (err == 10061) { // Connection refused - server not ready yet
					retries++;
					printf("[TCP_Thread] Connection refused, retry %d/%d...\n", retries, MAX_RETRIES);
					Sleep(500); // Wait 500ms before retry
					continue;
				}
				printf("[TCP_Thread] connect failed: %d\n", err);
				closesocket(pThis->m_tcpSocket);
				pThis->m_tcpSocket = INVALID_SOCKET;
				return -1;
			}
			break; // Connected successfully
		}
		
		if (retries >= MAX_RETRIES) {
			printf("[TCP_Thread] Failed to connect after %d retries\n", MAX_RETRIES);
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}
		
		printf("[TCP_Thread] Connected to user %s!\n\n", pThis->m_peerUsername);
	}

	// set state to connected and start heartbeat timer
//	pThis->StartTimer(TIMER2_ID);
	pThis->SetState(CONNECTED);
	
	char recvBuf[BUFFER_SIZE];
	while (true) {
		int bytesRecv = recv(pThis->m_tcpSocket, recvBuf, BUFFER_SIZE - 1, 0);
		if (bytesRecv > 0) {
			recvBuf[bytesRecv] = '\0';
	//		pThis->StopTimer(TIMER2_ID); // Stop heartbeat timer on message receive
		//	pThis->StartTimer(TIMER2_ID);
			printf("\n[TCP_THREAD] Message from user [%s]: %s\n> ", pThis->m_peerUsername, recvBuf);
		}
		else {
			printf("[TCP_THREAD] %s connection closed: %d\n", pThis->m_peerUsername, WSAGetLastError());
			break;
		}
	}
	
	// cleanup after connection closes
	closesocket(pThis->m_tcpSocket);
	pThis->m_tcpSocket = INVALID_SOCKET;
	pThis->SetState(IDLE);
	
	return 0;
}

void TCPComs::ConnectionClosed() {
	SetState(IDLE);
	FreeFSM();  // Return to the thread free pool
}
void TCPComs::SendMSG(){
	//function to send messages over TCP

}
void TCPComs::ReceiveMSG(){
	//function to receive messages over TCP
}
void TCPComs::HandleSendData() {
	//get data from param and send over TCP socket

	//check if we're connected before trying to send
	if (GetState() != CONNECTED) {
		return;  // Silently ignore if not connected
	}
	uint8* dataParam = GetParam(PARAM_DATA);

	if (dataParam && m_tcpSocket != INVALID_SOCKET) {
		// First 4 bytes are FSM kernel parameter header, actual data starts at offset 4
		char* text = (char*)(dataParam + 4);

		// Send it over the actual TCP socket
		int result = send(m_tcpSocket, text, strlen(text), 0);

		if (result == SOCKET_ERROR) {
			printf("[TCP] Send failed: %d\n", WSAGetLastError());
		}
	}
}
void TCPComs::SetPeerInfo(const char* ip, const char* name, bool server) {
	strncpy(this->m_peerIP, ip, 15);
	this->m_peerIP[15] = '\0';
	strncpy(this->m_peerUsername, name, BUFFER_SIZE - 1);
	this->m_peerUsername[BUFFER_SIZE - 1] = '\0';
	this->m_isServer = server;
}
void TCPComs::OnTimerExpired() {

	//send heartbeat message to peer to check if connection is still alive. If send fails, close connection and return to idle state
	if (GetState() == CONNECTED) {
		printf("[TCP_FSM] Heartbeat message sent for %s\n", m_peerUsername);
		char* heartbeatMsg = "HEARTBEAT"; 
		int result = send(m_tcpSocket, heartbeatMsg, strlen(heartbeatMsg), 0);
		if (result == SOCKET_ERROR) {
			printf("[TCP_FSM] Failed to send heartbeat: %d\n", WSAGetLastError());
			ConnectionClosed();
			SetState(IDLE);
		}
	}
}

