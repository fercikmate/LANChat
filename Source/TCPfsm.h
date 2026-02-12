#pragma once

#include "../kernel/fsmSystem.h"
#include "../kernel/stdMsgpc16pl16.h"
#include "../kernel/TransportInterface.h"




#define TCP_MB	1
#define TCP_FSM	1
#define BUFFER_SIZE 512

#define MSG_USER_INPUT          0x0003
#define MSG_TCP_SHUTDOWN        0x0008
#define MSG_TCP_THREAD_START	0x0009
#define IS_SERVER_PARAM        0x000A

#define PARAM_USERNAME      0x01
#define PARAM_IP_ADDRESS    0x02
#define PARAM_PORT          0x03
#define PARAM_DATA          0x04

#define TIMER2_ID 1
#define TIMER2_COUNT 60
#define TIMER2_EXPIRED 0x21

typedef stdMsg_pc16_pl16 StandardMessage;

class TCPComs : public FiniteStateMachine {
	enum TCPComsStates { IDLE, CONNECTING_SERVER, CONNECTING_CLIENT, CONNECTED, SHUTDOWN_SENT, SHUTDOWN_REC, HEARTBEAT };

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
	DWORD dwListenerThreadID;

	bool m_isServer;
	char m_peerIP[16];
	char m_peerUsername[BUFFER_SIZE];
	uint16 m_peerPort;
	
	// Add these for the connection worker thread:
	HANDLE hConnectionThread;
	DWORD dwConnectionThreadID;

	static DWORD WINAPI TCPListenerThread(LPVOID param);

	static DWORD WINAPI ConnectionWorkerThread(LPVOID param);

	void TCPMsg_2_FSMMsg(const char* data, int length, sockaddr_in* sender);
	void CreateThread();
	void SendOk();
	void GotOk();
	void SendTCPBroadcast();
	void StartTCPListening();
	void ProcessConnectionRequest();
	void ConnectionClosed();
	void HandleSendData();
	void OnTimerExpired();


public:
	TCPComs();
	~TCPComs();

	void Initialize();
	void GetUsername();
	void Start();
	void Connecting();
	void SendMSG();
	void ReceiveMSG();

	void SetPeerInfo(const char* ip, const char* username, bool isServer);

	// Public accessors for checking existing connections
	const char* GetPeerUsername() const { return m_peerUsername; }
	const char* GetPeerIP() const { return m_peerIP; }
	bool IsConnectedToPeer() { return (m_tcpSocket != INVALID_SOCKET && GetState() == CONNECTED); }
};

// Helper function to check if connection exists (declare in header or a separate manager)
bool ConnectionExistsForPeer(const char* username, const char* ip);
