#include <stdio.h>

#include "const.h"
#include "ClAuto.h"

#define StandardMessageCoding 0x00

ClAuto::ClAuto() : FiniteStateMachine(CL_AUTOMATE_TYPE_ID, CL_AUTOMATE_MBX_ID, 0, 10, 2) {
}

ClAuto::~ClAuto() {
}


uint8 ClAuto::GetAutomate() {
	return CL_AUTOMATE_TYPE_ID;
}

/* This function actually connnects the ClAutoe with the mailbox. */
uint8 ClAuto::GetMbxId() {
	return CL_AUTOMATE_MBX_ID;
}

uint32 ClAuto::GetObject() {
	return GetObjectId();
}

MessageInterface* ClAuto::GetMessageInterface(uint32 id) {
	return &StandardMsgCoding;
}

void ClAuto::SetDefaultHeader(uint8 infoCoding) {
	SetMsgInfoCoding(infoCoding);
	SetMessageFromData();
}

void ClAuto::SetDefaultFSMData() {
	SetDefaultHeader(StandardMessageCoding);
}

void ClAuto::NoFreeInstances() {
	printf("[%d] ClAuto::NoFreeInstances()\n", GetObjectId());
}

void ClAuto::Reset() {
	printf("[%d] ClAuto::Reset()\n", GetObjectId());
}


void ClAuto::Initialize() {
	SetState(FSM_Cl_Ready);

	//intitialization message handlers
	InitEventProc(FSM_Cl_Ready, MSG_User_Check_Mail, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Ready_User_Check_Mail);
	InitEventProc(FSM_Cl_Connecting, MSG_Cl_Connection_Reject, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Connecting_Cl_Connection_Reject);
	InitEventProc(FSM_Cl_Connecting, MSG_Cl_Connection_Accept, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Connecting_Cl_Connectiong_Accept);
	InitEventProc(FSM_Cl_Authorising, MSG_User_Name_Password, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Authorising_User_Name_Password);
	InitEventProc(FSM_Cl_User_Check, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_User_Check_MSG);
	InitEventProc(FSM_Cl_Pass_Check, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Pass_Check_MSG);
	InitEventProc(FSM_Cl_Mail_Check, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Mail_Check_MSG);
	InitEventProc(FSM_Cl_Receiving, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Receiving_MSG);
	InitEventProc(FSM_Cl_Deleting, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Deleting_MSG);
	InitEventProc(FSM_Cl_Reseting, MSG_MSG, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Reseting_MSG);
	InitEventProc(FSM_Cl_Disconnecting, MSG_Cl_Disconected, (PROC_FUN_PTR)&ClAuto::FSM_Cl_Disconnecting_Cl_Disconected);

}

void ClAuto::FSM_Cl_Ready_User_Check_Mail() {

	printf("Connecting to %s ...\n", ADRESS);

	PrepareNewMessage(0x00, MSG_Connection_Request);
	SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(CH_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_Connecting);
}

void ClAuto::FSM_Cl_Connecting_Cl_Connection_Reject() {

	PrepareNewMessage(0x00, MSG_User_Connecton_Fail);
	SetMsgToAutomate(USER_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(USER_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_Ready);
}

void ClAuto::FSM_Cl_Connecting_Cl_Connectiong_Accept() {

	PrepareNewMessage(0x00, MSG_User_Connected);
	SetMsgToAutomate(USER_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(USER_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_Authorising);
}

void ClAuto::FSM_Cl_Authorising_User_Name_Password() {

	//char* name = new char[20];
	//char* pass = new char[20];

	uint8* buffer = GetParam(PARAM_Name);
	memcpy(m_UserName, buffer + 4, buffer[2]);
	m_UserName[buffer[2]] = 0; // terminate string
	buffer = GetParam(PARAM_Pass);
	memcpy(m_Password, buffer + 4, buffer[2]);
	m_Password[buffer[2]] = 0; // terminate string

	char l_Command[20] = "user ";
	strcpy(l_Command + 5, m_UserName);
	strcpy(l_Command + 5 + strlen(m_UserName), "\r\n");

	PrepareNewMessage(0x00, MSG_Cl_MSG);
	SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	AddParam(PARAM_DATA, strlen(l_Command), (uint8*)l_Command);
	SendMessage(CH_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_User_Check);
}

void ClAuto::FSM_Cl_User_Check_MSG() {

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	data[size] = 0;
	printf("%s", data);
	if (data[0] == '+')
	{
		char l_Command[20] = "pass ";
		strcpy(l_Command + 5, m_Password);
		strcpy(l_Command + 5 + strlen(m_Password), "\r\n");

		PrepareNewMessage(0x00, MSG_Cl_MSG);
		SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
		SetMsgObjectNumberTo(0);
		AddParam(PARAM_DATA, strlen(l_Command), (uint8*)l_Command);
		SendMessage(CH_AUTOMATE_MBX_ID);

		SetState(FSM_Cl_Pass_Check);
	}
	else {
		SendQuit();
	}

}

void ClAuto::FSM_Cl_Pass_Check_MSG() {

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	data[size] = 0;
	printf("%s", data);

	if (data[0] == '+')
	{
		char l_Command[20] = "stat\r\n";

		PrepareNewMessage(0x00, MSG_Cl_MSG);
		SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
		SetMsgObjectNumberTo(0);
		AddParam(PARAM_DATA, strlen(l_Command), (uint8*)l_Command);
		SendMessage(CH_AUTOMATE_MBX_ID);

		//SetState(FSM_Cl_Pass_Check);
		SetState(FSM_Cl_Mail_Check);
	}
	else {
		SendQuit();
	}

	//SetState(FSM_Cl_Mail_Check);//CHK
}

void ClAuto::FSM_Cl_Mail_Check_MSG() {

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	data[size] = 0;
	printf("%s", data);

	int l_nDigit = 1;
	while (buffer[l_nDigit + 8] != ' ') l_nDigit++;
	memcpy(data, buffer + 8, l_nDigit);
	data[l_nDigit] = 0;

	m_MessageCount = atoi(data);

	if (m_MessageCount == 0)
	{
		SendQuit();
	}
	else {

		//m_bNewSession = true;

		char l_Command[20] = "retr ";
		strcpy(l_Command + 5, data);
		strcpy(l_Command + 5 + l_nDigit, "\r\n");

		PrepareNewMessage(0x00, MSG_Cl_MSG);
		SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
		SetMsgObjectNumberTo(0);
		AddParam(PARAM_DATA, 5 + l_nDigit + 2, (uint8*)l_Command);
		SendMessage(CH_AUTOMATE_MBX_ID);

		SetState(FSM_Cl_Receiving);
	}
}

void ClAuto::FSM_Cl_Receiving_MSG() {

	char* data = new char[255];
	uint8* buffer = GetParam(PARAM_DATA);
	uint16 size = buffer[2];

	memcpy(data, buffer + 4, size);
	char temp[4];
	memcpy(temp, data, 3); temp[3] = 0;
	if (strcmp(temp, "+OK") != 0) {    // Ovde da stoji == bi uhvatili prvi deo od 255 char-ova od fajla 
		//data[size]=0;                  // a sa ovim hvatamo sve ostale 255-bajtne chunk-ove iz fajla jer  
		//printf("%s",data);             // oni nemaju "+OK" na svom pocetku
		PrepareNewMessage(0x00, MSG_Mail);
		SetMsgToAutomate(USER_AUTOMATE_TYPE_ID);
		SetMsgObjectNumberTo(0);
		AddParam(PARAM_DATA, size, (uint8*)data);
		SendMessage(USER_AUTOMATE_MBX_ID);
		//if(!m_bNewSession){


		if (size < 255)
		{

			char l_Command[20] = "dele ";
			itoa(m_MessageCount, data, 10);
			strcpy(l_Command + 5, data);
			strcpy(l_Command + 5 + strlen(data), "\r\n");

			PrepareNewMessage(0x00, MSG_Cl_MSG);
			SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
			SetMsgObjectNumberTo(0);
			AddParam(PARAM_DATA, 5 + strlen(data) + 2, (uint8*)l_Command);
			SendMessage(CH_AUTOMATE_MBX_ID);

			SetState(FSM_Cl_Deleting);
		}
	}

}

void ClAuto::FSM_Cl_Deleting_MSG() {

	PrepareNewMessage(0x00, MSG_User_Save_Mail);
	SetMsgToAutomate(USER_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(USER_AUTOMATE_MBX_ID);

	m_MessageCount--;
	if (m_MessageCount > 0) {
		char data[5];
		char l_Command[20] = "retr ";
		itoa(m_MessageCount, data, 10);
		strcpy(l_Command + 5, data);
		strcpy(l_Command + 5 + strlen(data), "\r\n");

		PrepareNewMessage(0x00, MSG_Cl_MSG);
		SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
		SetMsgObjectNumberTo(0);
		AddParam(PARAM_DATA, 5 + strlen(data) + 2, (uint8*)l_Command);
		SendMessage(CH_AUTOMATE_MBX_ID);

		SetState(FSM_Cl_Receiving);
	}
	else {
		SendQuit();
	}
}

void ClAuto::FSM_Cl_Reseting_MSG() {

	SendQuit();
}

void ClAuto::SendQuit() {
	char l_Command[20] = "quit\r\n";

	PrepareNewMessage(0x00, MSG_Cl_MSG);
	SetMsgToAutomate(CH_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	AddParam(PARAM_DATA, strlen(l_Command), (uint8*)l_Command);
	SendMessage(CH_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_Disconnecting);
}

void ClAuto::FSM_Cl_Disconnecting_Cl_Disconected() {

	PrepareNewMessage(0x00, MSG_User_Disconnected);
	SetMsgToAutomate(USER_AUTOMATE_TYPE_ID);
	SetMsgObjectNumberTo(0);
	SendMessage(USER_AUTOMATE_MBX_ID);

	SetState(FSM_Cl_Ready);
}
