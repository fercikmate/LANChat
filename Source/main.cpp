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


#define CLIENT_ADDRESS "127.0.0.1"		// IPv4 address of server
#define SERVER_PORT 27015					// Port number of server that will be used for communication with clients
#define BUFFER_SIZE 512						// Size of buffer that will be used for sending and receiving messages to client
#define serverPort 8080
int getUserName(char* username);
SOCKET setupServerSocket()
{
	struct sockaddr_in servAddr;
	WSADATA wsaData;
	// Initialize Winsock library
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	// Check if library is succesfully initialized
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return -1;
	}

	// Clear before set up server address structure
	memset(&servAddr, 0, sizeof(servAddr));

	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servAddr.sin_port = htons(serverPort);

	
	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if (serverSocket == INVALID_SOCKET)
	{
		printf("Creating socket failed with error: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		return -1;
	}

	if (bind(serverSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0) {
		printf("Error binding UDP socket\n");
		closesocket(serverSocket);
		return -1;
	}
	printf("UDP server is up and running! Listening on port: %d...\n", serverPort);
	return serverSocket;
}

int sendUDPalive() {
	struct sockaddr_in cliAddr;
	cliAddr.sin_family = AF_INET;
	cliAddr.sin_addr.s_addr = inet_addr(CLIENT_ADDRESS);
	cliAddr.sin_port = htons(SERVER_PORT);
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udpSocket == INVALID_SOCKET) {
		printf("Creating UDP socket failed with error: %d\n", WSAGetLastError());
		return -1;
	}
	char username[BUFFER_SIZE];
	if (getUserName(username) == -1) {
		printf("Failed to get username");
		_getch();
		return -1;
	}


	if (sendto(udpSocket, username, (int)strlen(username), 0, (struct sockaddr*)&cliAddr, sizeof(cliAddr)) == SOCKET_ERROR) {
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
{
	//Set up server
	SOCKET serverSocket = setupServerSocket();
	if (serverSocket == -1) {
		printf("Press a button to exit the application due to socket setup failure.\n");
		char ch = _getch();
		WSACleanup();
		return -1;
	}
	if (sendUDPalive() == -1) {
		printf("Press a button to exit the application due to failing to send ALIVE msg.\n");
		char ch = _getch();
		closesocket(serverSocket);
		WSACleanup();
	}
	//Declare clientaddress for recvfrom
	char dataBuffer[BUFFER_SIZE];
	sockaddr_in6 clientAddress;
	memset(&clientAddress, 0, sizeof(clientAddress));

	// Set whole buffer to zero
	memset(dataBuffer, 0, BUFFER_SIZE);

	// size of client address
	int sockAddrLen = sizeof(clientAddress);
	printf("===========================\n");
	printf("Awaiting connections...");
	if (recvfrom(serverSocket, dataBuffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddress, &sockAddrLen) < 0) {
		printf("Error receiving data on UDP socket\n");
		closesocket(serverSocket);
		WSACleanup();
		return -1;
	}
	//TODO: worker thread to 
	AutoExample autoExample;
	autoExample.Initialize();
	autoExample.Start();
	printf("AutoExample FSM started. Press any key to exit...\n");
	char ch = _getch(); // Wait for user input before exiting
	int close(int fd);
	return 0;
}