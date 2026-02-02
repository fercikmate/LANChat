#include <stdio.h>
#include "ClientSearchFSM.h"

AutoExample::AutoExample() : FiniteStateMachine(AUTOEXAMPLE_FSM, AUTOEXAMPLE_MBX_ID, 10, 10, 10) {
}

AutoExample::~AutoExample() {
}


uint8 AutoExample::GetAutomate() {
	return AUTOEXAMPLE_FSM;
}

/* This function actually connnects the AutoExamplee with the mailbox. */
uint8 AutoExample::GetMbxId() {
	return AUTOEXAMPLE_MBX_ID;
}

MessageInterface* AutoExample::GetMessageInterface(uint32 id) {
	if (id == 0)
		return &StandardMsgCoding;
	throw TErrorObject(__LINE__, __FILE__, 0x01010400);
}

void AutoExample::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void AutoExample::SetDefaultFSMData() {

}

void AutoExample::NoFreeInstances() {
	printf("[%d] AutoExample::NoFreeInstances()\n", GetObjectId());
}

void AutoExample::Initialize() {
	SetState(AUTO_STATE0);
	SetDefaultFSMData();
}

/* Initial system message */
void AutoExample::Start() {
}
