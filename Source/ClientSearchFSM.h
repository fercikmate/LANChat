#pragma once

#include "../kernel/fsmSystem.h"
#include "../kernel/stdMsgpc16pl16.h"



#define AUTOEXAMPLE_MBX_ID    0
#define AUTOEXAMPLE_FSM       0

typedef stdMsg_pc16_pl16 StandardMessage1;

class AutoExample : public FiniteStateMachine {
	enum AutoExampleStates { AUTO_STATE0, AUTO_STATE1, AUTO_STATE2 };

	StandardMessage StandardMsgCoding;

	/* FiniteStateMachine abstract functions */
	MessageInterface* GetMessageInterface(uint32 id);
	void	SetDefaultHeader(uint8 infoCoding);
	void	SetDefaultFSMData();
	void	NoFreeInstances();
	uint8	GetMbxId();
	uint8	GetAutomate();

public:
	AutoExample();
	~AutoExample();

	void Initialize();
	void Start();
};
