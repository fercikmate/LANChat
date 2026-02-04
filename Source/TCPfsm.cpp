#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include <stdio.h>	
#include <string.h>
#include <winsock2.h>
#include "TCPfsm.h"

#define CLIENT_ADDRESS "255.255.255.255"
#define SERVER_PORT 8080

extern char username[BUFFER_SIZE];

TCPComs::TCPComs() : FiniteStateMachine(TCP_FSM, TCP_MB, 10, 10, 10) {
}

TCPComs::~TCPComs() {
}


uint8 TCPComs::GetAutomate() {
	return TCP_FSM;
}

/* This function actually connnects the AutoExamplee with the mailbox. */
uint8 TCPComs::GetMbxId() {
	return TCP_MB;
}

MessageInterface* TCPComs::GetMessageInterface(uint32 id) {
	if (id == 0)
		return &StandardMsgCoding;
	throw TErrorObject(__LINE__, __FILE__, 0x01010400);
}

void TCPComs::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void TCPComs::SetDefaultFSMData() {

}

void TCPComs::NoFreeInstances() {
	printf("[%d] TCPComs::NoFreeInstances()\n", GetObjectId());
}

void TCPComs::Initialize() {
	SetState(CONNECTING_CLIENT);

}
void TCPComs::Start() {
	printf("[%d] TCPComs::Start() called\n", GetObjectId());
}
void TCPComs::Connecting() {
	printf("[%d] TCPComs::Start() called\n", GetObjectId());
}

