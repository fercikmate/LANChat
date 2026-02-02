//RA177/2020
//This file starts the process and monitors incoming udp connections
//Then it spawns a new thread to handle each connection
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include "ClientTCPfsm.h"


#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib") 
#pragma comment (lib, "AdvApi32.lib") 

#define CLIENT_ADDRESS "255.255.255.255"		// IPv4 address of server
#define SERVER_PORT 8080					// Port
#define BUFFER_SIZE 512						//Max msg size

int getUserName(char* username);

int initializeWSA() {
	WSADATA wsaData;
	// Initialize Winsock library
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Check if library is succesfully initialized
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return -1;
	}
	return 0;
}

SOCKET setupServerSocket()
{
	struct sockaddr_in servAddr;
	

	// Clear before set up server address structure
	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servAddr.sin_port = htons(SERVER_PORT);

	
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (serverSocket == INVALID_SOCKET)
	{
		printf("Creating socket failed with error: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		return -1;
	}
	int reuse = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
		(char*)&reuse, sizeof(reuse)) < 0) {
		printf("setsockopt SO_REUSEADDR failed: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	if (bind(serverSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		printf("Error binding UDP socket, %d\n", WSAGetLastError());
		closesocket(serverSocket);
		return -1;
	}
	
	printf("UDP server is up and running! Listening on port: %d...\n", SERVER_PORT);
	
	return serverSocket;
}

int sendUDPalive(char* username) {
	struct sockaddr_in broadcastAddr;
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
	broadcastAddr.sin_port = htons(SERVER_PORT);
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		printf("Creating UDP socket failed with error: %d\n", WSAGetLastError());
		return -1;
	}
	 
	
	int broadcast = 1;
	if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
		(char*)&broadcast, sizeof(broadcast)) < 0) {
		printf("setsockopt SO_BROADCAST failed: %d\n", WSAGetLastError());
		closesocket(udpSocket);
		return -1;
	}


	if (sendto(udpSocket, username, (int)strlen(username), 0, (struct sockaddr*)&broadcastAddr, sizeof(broadcastAddr)) == SOCKET_ERROR) {
		printf("sendto failed with error: %d\n", WSAGetLastError());
		closesocket(udpSocket);
		return -1;
	}
	printf("Alive msg sent with username %s\n", username);
	
	return 0;
}

int getUserName(char* username) {
	printf("===========================\n");
	printf("Welcome! Enter your username: \n");
	printf("===========================\n");

	if (fgets(username, BUFFER_SIZE, stdin) == NULL) {
		printf("Error reading username\n");
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
		printf("Username cannot be empty\n");
		return -1;
	}

	printf("Username set to: %s\n", username);
	return 0;
}

int main()
{	//Initialize WSA
	if (initializeWSA() == -1) {
		printf("Press a button to exit the application due to WSAStartup failure.\n");
		char ch = _getch();
		return -1;
	}
	char username[BUFFER_SIZE];
	while (getUserName(username) == -1) {
		printf("Failed to get username");
		char ch = _getch();
	}
	
	//Send UDP ALIVE msg with username
	if (sendUDPalive(username) == -1) {
		printf("Press a button to exit the application due to failing to send ALIVE msg.\n");
		char ch = _getch();
		WSACleanup();
	}
	//Set up server
	SOCKET serverSocket = setupServerSocket();
	if (serverSocket == -1) {
		printf("Press a button to exit the application due to socket setup failure.\n");
		char ch = _getch();
		WSACleanup();
		return -1;
	}

	//Declare clientaddress for recvfrom
	char dataBuffer[BUFFER_SIZE];
	sockaddr_in clientAddress;
	memset(&clientAddress, 0, sizeof(clientAddress));

	// Set whole buffer to zero
	memset(dataBuffer, 0, BUFFER_SIZE);

	// size of client address
	int sockAddrLen = sizeof(clientAddress);
	printf("===========================\n");
	printf("Awaiting connections...");
	while (true)
	{

	
	if (recvfrom(serverSocket, dataBuffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddress, &sockAddrLen) < 0) {
		printf("Error receiving data on UDP socket\n");
		closesocket(serverSocket);
		WSACleanup();
		return -1;
	}
	else printf("Connection established with client: %s\n", dataBuffer);
	}
	//TODO: worker thread to  
	//AutoExample autoExample;
	//autoExample.Initialize();
	//autoExample.Start();
	//printf("AutoExample FSM started. Press any key to exit...\n");
	char ch = _getch(); // Wait for user input before exiting
	//int close(int fd);
	return 0;
}