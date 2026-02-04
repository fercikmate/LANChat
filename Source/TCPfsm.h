#pragma once

#include "../kernel/fsmSystem.h"
#include "../kernel/stdMsgpc16pl16.h"
#include "../kernel/TransportInterface.h"




#define TCP_MB	0
#define TCP_FSM	0
#define BUFFER_SIZE 512

#define MSG_TCP_ALIVE_RECEIVED  0x0001
#define MSG_TCP_OK_RECEIVED     0x0002
#define MSG_USER_INPUT          0x0003
#define MSG_TCP_CONNECTED       0x0004
#define MSG_TCP_MESSAGE         0x0005

#define PARAM_USERNAME      0x01
#define PARAM_IP_ADDRESS    0x02
#define PARAM_PORT          0x03
#define PARAM_DATA          0x04

typedef stdMsg_pc16_pl16 StandardMessage;

class TCPComs : public FiniteStateMachine {
	enum TCPComsStates { CONNECTING_SERVER, CONNECTING_CLIENT, CONNECTED, SHUTDOWN_SENT, SHUTDOWN_REC, HEARTBEAT };

	StandardMessage StandardMsgCoding;

	/* FiniteStateMachine abstract functions */
	MessageInterface* GetMessageInterface(uint32 id);
	void	SetDefaultHeader(uint8 infoCoding);
	void	SetDefaultFSMData();
	void	NoFreeInstances();
	uint8	GetMbxId();
	uint8	GetAutomate();


private:
	SOCKET m_tcpSocket;
	HANDLE ListenerThread;
	DWORD ThreadID;
	static DWORD WINAPI TCPListenerThread(LPVOID param);
	void TCPMsg_2_FSMMsg(const char* data, int length, sockaddr_in* sender);
	void CreateThread();
	void SendOk();
	void GotOk();
	void SendTCPBroadcast();
	void StartTCPListening();
	


public:
	TCPComs();
	~TCPComs();

	void Initialize();
	void GetUsername();
	void Start();
	void Connecting();



};
