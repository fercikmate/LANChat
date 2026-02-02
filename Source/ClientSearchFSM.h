#pragma once

#include "../kernel/fsm.h"
#include "../kernel/fsmsystem.h"

#include "../kernel/stdMsgpc16pl16.h"
#include "../kernel/NetFSM.h"


class ClAuto : public FiniteStateMachine {

	// for FSM
	StandardMessage StandardMsgCoding;

	MessageInterface* GetMessageInterface(uint32 id);
	void	SetDefaultHeader(uint8 infoCoding);
	void	SetDefaultFSMData();
	void	NoFreeInstances();
	void	Reset();
	uint8	GetMbxId();
	uint8	GetAutomate();
	uint32	GetObject();
	void	ResetData();


public:
	ClAuto();
	~ClAuto();

	void Initialize();

	void FSM_Cl_Ready_User_Check_Mail();
	void FSM_Cl_Connecting_Cl_Connection_Reject();
	void FSM_Cl_Connecting_Cl_Connectiong_Accept();
	void FSM_Cl_Authorising_User_Name_Password();
	void FSM_Cl_User_Check_MSG();
	void FSM_Cl_Pass_Check_MSG();
	void FSM_Cl_Mail_Check_MSG();
	void FSM_Cl_Receiving_MSG();
	void FSM_Cl_Deleting_MSG();
	void FSM_Cl_Reseting_MSG();
	void FSM_Cl_Disconnecting_Cl_Disconected();
	void SendQuit();

protected:

	int m_MessageCount;
	char m_UserName[20];
	char m_Password[20];
};


