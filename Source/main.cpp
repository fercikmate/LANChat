//RA177/2020
//This file starts the process and monitors incoming udp connections
//Then it spawns a new thread to handle each connection
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <conio.h>
#include "ClientSearchFSM.h"
#include "../kernel/logFile.h"
#include "TCPfsm.h"


#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib") 
#pragma comment (lib, "AdvApi32.lib") 

#define BUFFER_SIZE 512						//Max msg size
HANDLE hFsmMutex = CreateMutex(NULL, FALSE, NULL); // Mutex for thread safety
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

/* FSM system instance. */
FSMSystem Client_sys(2 /* max number of automates types */, 2 /* max number of msg boxes */);
DeviceSearch udpAutomate;
TCPComs tcpPool[10];
TCPComs* tcpInstances = tcpPool;
int tcpInstanceCount = 10;

DWORD WINAPI SystemThread(void* data) {
	

	const uint8 buffClassNo = 4;
	uint32 buffsCount[buffClassNo] = { 50, 50, 50, 10 };
	uint32 buffsLength[buffClassNo] = { 128, 256, 512, 1024 };

	LogFile lf("log.log", "./log.ini");
	LogAutomateNew::SetLogInterface(&lf);

	printf("[*] Initializing system...\n");
	Client_sys.InitKernel(buffClassNo, buffsCount, buffsLength, 5);

	/* Add UDP automate */
	Client_sys.Add(&udpAutomate, UDP_FSM, 1, true);

	Client_sys.Add(&tcpPool[0], TCP_FSM, 10, true);
	/* Pre-allocate all 10 TCP automates from the pool */
	for (int i = 1; i < 10; i++) {
		Client_sys.Add(&tcpPool[i], TCP_FSM);
	}

	udpAutomate.Start();
	Client_sys.Start();

	/* Finish thread */
	return 0;
}

DWORD WINAPI ConsoleInputThread(LPVOID param) {
	char inputBuffer[BUFFER_SIZE];
	while (true) {
		if (fgets(inputBuffer, BUFFER_SIZE, stdin)) {
			inputBuffer[strcspn(inputBuffer, "\n")] = 0;

			// Simple and clean: let the object handle its own messaging
			WaitForSingleObject(hFsmMutex, INFINITE);
			udpAutomate.SendUserInput(inputBuffer);
			ReleaseMutex(hFsmMutex);
		}
	}
	return 0;
}
char username[BUFFER_SIZE];
int myTcpPort = 8081;


int main()
{	
	//Initialize WSA
	if (initializeWSA() == -1) {	
		printf("Press a button to exit the application due to WSAStartup failure.\n");
		char ch = _getch();
		return -1;
	}
	if (DeviceSearch::getUsername() == -1) {
		printf("Press a button to exit the application due to invalid username.\n");
		char ch = _getch();
		return -1;
	}

	DWORD thread_id;
	HANDLE thread_handle;

	/* Start operating thread - FSM handles everything */
	thread_handle = CreateThread(NULL, 0, SystemThread, NULL, 0, &thread_id);

	HANDLE hConsole = CreateThread(NULL, 0, ConsoleInputThread, NULL, 0, NULL);
	if (hConsole == NULL) {
		printf("[!] Failed to start Console Thread\n");
	}

	// Keep main thread alive
	while (true) { Sleep(1000); }
	/* Wait for the FSM thread to complete (it runs indefinitely) */

	WaitForSingleObject(thread_handle, INFINITE);

	/* Clean up */
	CloseHandle(thread_handle);
	WSACleanup();
	
	return 0;
}