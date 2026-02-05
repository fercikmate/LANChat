#pragma once

#include "../kernel/fsmSystem.h"
#include "../kernel/stdMsgpc16pl16.h"
#include "../kernel/TransportInterface.h"
#include <conio.h>



#define SERVER_PORT 8080
#define UDP_MB	0
#define UDP_FSM	0
#define BUFFER_SIZE 512

#define MSG_UDP_ALIVE_RECEIVED  0x0001
#define MSG_UDP_OK_RECEIVED     0x0002
#define MSG_USER_INPUT          0x0003
#define MSG_UDP_CONNECTED       0x0004
#define MSG_UDP_MESSAGE         0x0005

//timer resolution is 1 second, so TIMER1_COUNT of 10 means 10 seconds
#define TIMER1_ID 0
#define TIMER1_COUNT 15 
#define TIMER1_EXPIRED 0x20 

#define PARAM_USERNAME      0x01
#define PARAM_IP_ADDRESS    0x02
#define PARAM_PORT          0x03
#define PARAM_DATA          0x04

typedef stdMsg_pc16_pl16 StandardMessage;

class DeviceSearch : public FiniteStateMachine {
enum DeviceSearchStates { LOGIN, IDLE };

	StandardMessage StandardMsgCoding;

	/* FiniteStateMachine abstract functions */
	MessageInterface* GetMessageInterface(uint32 id);
	void	SetDefaultHeader(uint8 infoCoding);
	void	SetDefaultFSMData();
	void	NoFreeInstances();
	uint8	GetMbxId();
	uint8	GetAutomate();



private:
	SOCKET m_udpSocket;
	HANDLE ListenerThread;
	DWORD ThreadID;
	static DWORD WINAPI UdpListenerThread(LPVOID param);
	void UdpMsg_2_FSMMsg(const char* data, int length, sockaddr_in* sender);
	void CreateThread();
	void SendOk();
	void GotOk();
	void SendUdpBroadcast();
	void StartUDPListening();
	DWORD WINAPI ConnectionThread(void* data);
	void createConnectionInstance(const char* peerIP, const char* peerUsername, uint16 port, bool server);
	void SendOkDirect(const char* peerUsername, const char* peerIP);
	void GotOkDirect(const char* peerUsername, const char* peerIP);
	static DWORD WINAPI ConsoleInputThread(LPVOID param);
	void OnTimerExpired();
	int m_retryCount;           // Track retry attempts
	bool m_consoleThreadStarted; // Flag to start console thread only once
public:
	DeviceSearch();
	~DeviceSearch();

	void Initialize();
	void GetUsername();
	void Start();
	void SendUserInput(const char* text);
	static int getUsername();
	

};

