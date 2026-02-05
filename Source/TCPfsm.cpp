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
	else {
		// Connect logic

		printf("[TCP_Thread] Connecting to user %s at %s:%d...\n\n",
		pThis->m_peerUsername, pThis->m_peerIP, pThis->m_peerPort);
		

		if (connect(pThis->m_tcpSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
			printf("[TCP_Thread] connect failed: %d\n", WSAGetLastError());
			closesocket(pThis->m_tcpSocket);
			pThis->m_tcpSocket = INVALID_SOCKET;
			return -1;
		}
		
		printf("[TCP_Thread] Connected to user %s!\n\n", pThis->m_peerUsername);
	}

	// Update state safely or just start the send/recv loop here.
	pThis->SetState(CONNECTED);
	char recvBuf[BUFFER_SIZE];
	while (true) {
		int bytesRecv = recv(pThis->m_tcpSocket, recvBuf, BUFFER_SIZE - 1, 0);
		if (bytesRecv > 0) {
			recvBuf[bytesRecv] = '\0';
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
		// Offset 4 is where the actual string starts in this kernel's Param format
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