// UDP server that use non-blocking sockets, and receive messages from IPv4 and IPv6 clients.

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_PORT 27015	// Port number of server that will be used for communication with clients
#define BUFFER_SIZE 512		// Size of buffer that will be used for sending and receiving messages to clients

int main()
{
	// Server address
	sockaddr_in serverAddress1;
	sockaddr_in6 serverAddress2;

	// Buffer we will use to send and receive clients' messages
	char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	// Initialize serverAddress structure used by bind function for IPv4
	memset((char*)&serverAddress1, 0, sizeof(serverAddress1));
	serverAddress1.sin_family = AF_INET; 				// Set server address protocol family to IPv4
	serverAddress1.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses of server
	serverAddress1.sin_port = htons(SERVER_PORT);

	// Initialize serverAddress structure used by bind function for IPv6
	memset((char*)&serverAddress2, 0, sizeof(serverAddress2));
	serverAddress2.sin6_family = AF_INET6; 			    // Set server address protocol family to IPv6
	serverAddress2.sin6_addr = in6addr_any;			    // Use all available addresses of server
	serverAddress2.sin6_port = htons(SERVER_PORT);
	serverAddress2.sin6_flowinfo = 0;
	serverAddress2.sin6_scope_id = 0;

	// Create IPv4 socket
	SOCKET serverSocket1 = socket(AF_INET,      // IPv4 address family
		SOCK_DGRAM,   // datagram socket
		IPPROTO_UDP); // UDP
	
	// Create IPv6 socket
	SOCKET serverSocket2 = socket(AF_INET6,     // IPv6 address family
		SOCK_DGRAM,   // datagram socket
		IPPROTO_UDP); // UDP

	// Check if socket creation succeeded
	if (serverSocket1 == INVALID_SOCKET || serverSocket2 == INVALID_SOCKET)
	{
		printf("Creating socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Setting non-blocking mode on both sockets
	unsigned long mode = 1;
	int iResult1 = ioctlsocket(serverSocket1, FIONBIO, &mode);
	int iResult2 = ioctlsocket(serverSocket2, FIONBIO, &mode);

	if (iResult1 != NO_ERROR || iResult2 != NO_ERROR)
	{
		printf("ioctlsocket failed with error: %ld ili %ld\n", iResult1, iResult2);
		closesocket(serverSocket1);
		closesocket(serverSocket2);
		WSACleanup();
		return 1;
	}

	// Bind server address structure (type, port number and local address) to socket
	iResult1 = bind(serverSocket1,(SOCKADDR *)&serverAddress1, sizeof(serverAddress1));

	// Bind server address structure (type, port number and local address) to socket
	iResult2 = bind(serverSocket2,(SOCKADDR *)&serverAddress2, sizeof(serverAddress2));

	// Check if socket is succesfully binded to server datas
	if (iResult1 == SOCKET_ERROR || iResult2 == SOCKET_ERROR)
	{
		printf("Socket bind failed with error: %d\n", WSAGetLastError());
		closesocket(serverSocket1);
		closesocket(serverSocket2);
		WSACleanup();
		return 1;
	}

	printf("Simple UDP server started and waiting client messages.\n");

	// Main server loop
	while(1)
	{
		// Declare and initialize client address that will be set from recvfrom
		sockaddr_in clientAddress1;
		sockaddr_in6 clientAddress2;
		memset(&clientAddress1, 0, sizeof(clientAddress1));
		memset(&clientAddress2, 0, sizeof(clientAddress2));

		// Set whole buffer to zero
		memset(dataBuffer, 0, BUFFER_SIZE);

		// Size of IPv4 and IPv6 address structures
		int sockAddrLen1 = sizeof(clientAddress1);
		int sockAddrLen2 = sizeof(clientAddress2);

		char ipAddress1[16]; 
		char ipAddress2[INET6_ADDRSTRLEN]; 
		unsigned short clientPort;

		// Receive IPv4 client message
		iResult1 = recvfrom(serverSocket1,		// Own socket
			dataBuffer,							// Buffer that will be used for receiving message
			BUFFER_SIZE,						// Maximal size of buffer
			0,									// No flags
			(SOCKADDR *)&clientAddress1,		// Client information from received message (ip address and port)
			&sockAddrLen1);						// Size of sockadd_in structure

		// Check if message is succesfully received
		if (iResult1 != SOCKET_ERROR)
		{	
			// Copy client ip to local char[]
			strcpy_s(ipAddress1, sizeof(ipAddress1), inet_ntoa(clientAddress1.sin_addr));

			// Convert port number from network byte order to host byte order
			clientPort = ntohs(clientAddress1.sin_port);

			printf("Client connected from ip: %s, port: %d, sent: %s.\n", ipAddress1, clientPort, dataBuffer);
		}
		else{
			if (WSAGetLastError() != WSAEWOULDBLOCK){ 
				printf("recvfrom failed with error: %d\n", WSAGetLastError());
				continue;
			}
		}

		// Receive IPv6 client message
		iResult2 = recvfrom(serverSocket2,		// Own socket
			dataBuffer,							// Buffer that will be used for receiving message
			BUFFER_SIZE,						// Maximal size of buffer
			0,									// No flags
			(struct sockaddr *)&clientAddress2,	// Client information from received message (ip address and port)
			&sockAddrLen2);						// Size of sockadd_in structure

		// Check if message is succesfully received
		if (iResult2 != SOCKET_ERROR)
		{	

			// Copy client ip to local char[]
			inet_ntop(AF_INET6, &clientAddress2.sin6_addr, ipAddress2, INET6_ADDRSTRLEN);

			// Convert port number from network byte order to host byte order
			clientPort = ntohs(clientAddress2.sin6_port);

			printf("Client connected from ip: %s, port: %d, sent: %s.\n", ipAddress2, clientPort, dataBuffer);
		}
		else{
			if (WSAGetLastError() != WSAEWOULDBLOCK){
				printf("recvfrom failed with error: %d\n", WSAGetLastError());
				continue;
			}
		}

	}

	// Close server application
	iResult1 = closesocket(serverSocket1);
	iResult2 = closesocket(serverSocket2);
	if (iResult1 == SOCKET_ERROR || iResult2 == SOCKET_ERROR)
	{
		printf("closesocket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	printf("Server successfully shut down.\n");

	// Close Winsock library
	WSACleanup();

	return 0;
}
