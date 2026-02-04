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

DWORD WINAPI SystemThread(void* data) {
	DeviceSearch udpAutomate; //initialize UDP automate
	TCPComs tcpAutomate;  //initialize TCP automate

	/* Kernel buffer description block */
	/* number of buffer types */
	const uint8 buffClassNo = 4;
	/* number of buffers of each buffer type */
	uint32 buffsCount[buffClassNo] = { 50, 50, 50, 10 };
	/* buffer size for each buffer type */
	uint32 buffsLength[buffClassNo] = { 128, 256, 512, 1024 };

	/* Logging setting - to a file in this case */
	LogFile lf("log.log" /*log file name*/, "./log.ini" /* message translator file */);
	LogAutomateNew::SetLogInterface(&lf);

	/* Mandatory kernel initialization */
	printf("[*] Initializing system...\n");
	Client_sys.InitKernel(buffClassNo, buffsCount, buffsLength, 5);

	/* Add UDP automate - type 0 */
	Client_sys.Add(&udpAutomate, UDP_FSM, 1, true);
	
	/* Initialize TCP automate type with maximum of 10 slots */
	Client_sys.Add(&tcpAutomate, TCP_FSM, 10, true);

	/* Start the first automate - handles login, UDP alive, and server setup */
	udpAutomate.Start();

	
	Client_sys.Start();

	/* Finish thread */
	return 0;
}

char username[BUFFER_SIZE];

int main()
{	
	//Initialize WSA
	if (initializeWSA() == -1) {	
		printf("Press a button to exit the application due to WSAStartup failure.\n");
		char ch = _getch();
		return -1;
	}

	DWORD thread_id;
	HANDLE thread_handle;

	/* Start operating thread - FSM handles everything */
	thread_handle = CreateThread(NULL, 0, SystemThread, NULL, 0, &thread_id);

	/* Wait for the FSM thread to complete (it runs indefinitely) */

	WaitForSingleObject(thread_handle, INFINITE);

	/* Clean up */
	CloseHandle(thread_handle);
	WSACleanup();
	
	return 0;
}