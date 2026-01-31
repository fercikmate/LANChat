// **************************************************************************
//                        Client.cpp  -  description
//                           -------------------
//  begin                : Mon Nov 5 2001
//  coded by             : Aleksander Stojivcevic
//  e-mail               : sasa@krt.neobee.net
//  Description:
//  This file contains code for Client class used for receiving messages from net. 
//  Is used by some automate extended from NetFSM
// **************************************************************************

#include "NetFSM.h"
#include "Client.h"

// Function: Constructor
// Parameters: 
//  (in) SOCKET cl		- SOCKET throght data arrive.
//  (in) NetFSM net		- instance of NetFSM, needed for queuing arrived messages in FSM system.
//  
// Return value: none
// Description: 
//  Initialization of object Client.
Client::Client(SOCKET &cl, NetFSM *net)
{
	nrOfClients++;
	//if(nrOfClients == 1)
		//InitializeCriticalSection(&CriticalSection);
	client = cl;
	netFSM = net;
	ptrBuffer = new char[MAX_LENGTH_MESSAGE];


	currentState = STATE0;
	currentMsgLength = 0;
	priv = new unsigned char[MAX_LENGTH_MESSAGE];
	recvMsgLength = 0;
	
	send = false;
	haveMore = false;
	readFromNET = true;

	currentMsgLength = 0;

}

// Function: Destructor
// Parameters: none
//  
// Return value: none
// Description:
Client::~Client()
{
	nrOfClients--;
	//if(nrOfClients == 0)
		//DeleteCriticalSection(&CriticalSection);
	// dodato naknadno mislim da nije potrebno releaseAll();
	delete[] ptrBuffer;
	delete[] priv;
}


// Function: getError()
// Parameters: none
//  
// Return value: none
// Description: 
//  In debuging mode informs about errrors.
void Client::getError()
{
	int errorCode = WSAGetLastError();
#ifdef WIN32
	switch(errorCode)
	{
	case WSAENETDOWN:
		OutputDebugString((LPCWSTR)"The network subsystem has failed.\n");
		break;
	case WSAEFAULT:
		OutputDebugString((LPCWSTR)"The buf parameter is not completely contained in a valid part of the user address space.\n");
		break;
	case WSAENOTCONN:
		OutputDebugString((LPCWSTR)"The socket is not connected.\n");
		break;
	case WSAEINTR:
		OutputDebugString((LPCWSTR)"The (blocking) call was canceled through WSACancelBlockingCall.\n");
		break;
	case WSAEINPROGRESS:
		OutputDebugString((LPCWSTR)"A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.\n");
		break;
	case WSAENETRESET:
		OutputDebugString((LPCWSTR)"The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress.\n");
		break;
	case WSAENOTSOCK:
		OutputDebugString((LPCWSTR)"The descriptor is not a socket.\n");
		break;
	case WSAEOPNOTSUPP:
		OutputDebugString((LPCWSTR)"MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, out-of-band data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.\n");
		break;
	case WSAESHUTDOWN:
		OutputDebugString((LPCWSTR)"The socket has been shut down; it is not possible to recv on a socket after shutdown has been invoked with how set to SD_RECEIVE or SD_BOTH.\n");
		break;
	case WSAEWOULDBLOCK:
		OutputDebugString((LPCWSTR)"The socket is marked as nonblocking and the receive operation would block.\n");
		break;
	case WSAEMSGSIZE:
		OutputDebugString((LPCWSTR)"The message was too large to fit into the specified buffer and was truncated.\n");
		break;
	case WSAEINVAL:
		OutputDebugString((LPCWSTR)"The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.\n");
		break;
	case WSAECONNABORTED:
		OutputDebugString((LPCWSTR)"The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.\n");
		break;
	case WSAETIMEDOUT:
		OutputDebugString((LPCWSTR)"The connection has been dropped because of a network failure or because the peer system failed to respond.\n");
		break;
	case WSAECONNRESET:
		OutputDebugString((LPCWSTR)"The virtual circuit was reset by the remote side executing a hard or abortive close. The application should close the socket as it is no longer usable. On a UDP datagram socket this error would indicate that a previous send operation resulted in an ICMP Port Unreachable message.\n");
		break;
	}
#endif
}

// Function: releaseAll()
// Parameters: none
// Return value: none
// Description: 
//  Release resources like socket and kill thread.
void Client::releaseAll()
{
	if(this->client != 0)
		closesocket(this->client);
	this->stop();//ubija nit
	OutputDebugString((LPCWSTR)"Client is disconnected.\n");
	delete this;
}

// Function: workToDo()
// Parameters: none
// Return value: none
// Description: 
//  Here is used to handling arrived messages.
void Client::workToDo()
{
/*
begining my old
	int receivedBytes = 0;
	if(readFromNET)
	{
		
		receivedBytes = recv(client,ptrBuffer,MAX_LENGTH_MESSAGE,0);//poslednji parametar je nasumicno postavljen ne treba mi ni jedna od ponudjenih vrednosti
		if(receivedBytes == 0)
		{			
			releaseAll();
		}
		else if(receivedBytes > 0)
		{
			//uspesno primljena poruka treba je obraditi za sada nije potrebna kriticna sekcija koristice kada vise klijenata upisuje u isti MBX
			//EnterCriticalSection(&CriticalSection);
			
		}
		else
		{
			//doslo je do greske isfiltrirati koja je greska u pitanju
			//i u skladu sa time preuzeti dalje akcije
			getError();
			releaseAll();
		}
	}
	//begining - tcp stream control
	switch(currentState)
	{
		case STATE0:
			memcpy(&recvMsgLength,ptrBuffer,2);
			if(recvMsgLength > (receivedBytes-2))
			{
				memcpy(priv,ptrBuffer+2,receivedBytes-2);
				currentMsgLength = receivedBytes-2;
				currentState = STATE2;
			}
			else if(recvMsgLength < (receivedBytes-2))
			{
				memcpy(priv,ptrBuffer+2,recvMsgLength);
				currentMsgLength = 0;
				send = true;
				haveMore = true;
				readFromNET = false;
			}
			else
			{
				currentState = STATE0;
				send = true;
			}
			break;
		case STATE1:
			if(recvMsgLength == (receivedBytes-2))
			{
				send = true;
				haveMore = true;
				readFromNET = true;
				currentState = STATE0;
			}
			else if(recvMsgLength > (receivedBytes-2))
			{
				memcpy(priv,priv+2,receivedBytes-2);
				currentMsgLength = receivedBytes-2;
				currentState = STATE2;
				readFromNET = true;
			}
			if(recvMsgLength < (receivedBytes-2))
			{
				memcpy(priv,priv+2,recvMsgLength);
				currentMsgLength = 0;
				send = true;
				haveMore = true;
				readFromNET = false;
			}
			break;
		case STATE2:
			if(recvMsgLength == (currentMsgLength + receivedBytes))
			{
				memcpy(priv+currentMsgLength,ptrBuffer,receivedBytes);
				send = true;
				currentState = STATE0;
			}
			else if(recvMsgLength > (currentMsgLength + receivedBytes))
			{
				memcpy(priv+currentMsgLength,ptrBuffer,receivedBytes);
				currentMsgLength += receivedBytes;
				currentState = STATE2;
			}
			else if(recvMsgLength < (currentMsgLength + receivedBytes))
			{
				memcpy(priv+currentMsgLength,ptrBuffer,(recvMsgLength-currentMsgLength));
				send = true;
				haveMore = true;
				readFromNET = false;
			}
			break;
	}

	if(send)
	{
		netFSM->receivedMessageLength = recvMsgLength;
		recvMsgLength = 0;
		memcpy(netFSM->protocolMessageR,priv,recvMsgLength);
		netFSM->currentMessageCode = netFSM->convertNetToFSMMessage();
		netFSM->workWhenReceive(netFSM->getProtocolInfoCoding());
		//LeaveCriticalSection(&CriticalSection);
		send = false;
	}
	if(haveMore)
	{
		memcpy(priv,(ptrBuffer+((recvMsgLength-currentMsgLength))),receivedBytes-(recvMsgLength-currentMsgLength));
		memcpy(&recvMsgLength,(ptrBuffer+(recvMsgLength-currentMsgLength)),2);
		currentState = STATE1;
		haveMore = false;
	}

	//end - tcp stream control
end my old
*/
	int oldMessage = 0;//false
	int receivedBytes = 0;
	int receivedBytesMess = 0;
	receivedBytes = recv(client,ptrBuffer,2,0);//poslednji parametar je nasumicno postavljen ne treba mi ni jedna od ponudjenih vrednosti
	if(receivedBytes == 0)
	{			
    netFSM->NetError();
		releaseAll();
	}
	else if(receivedBytes > 0)
	{
		//uspesno primljena poruka treba je obraditi za sada nije potrebna kriticna sekcija koristice kada vise klijenata upisuje u isti MBX
		//EnterCriticalSection(&CriticalSection);
		unsigned short length = 0;

		unsigned char part1 = 0;
		unsigned char part2 = 0;
		part1 = ptrBuffer[0];
		part2 = ptrBuffer[1];
		length =0;
		length = 0x00ff & part1;
		length <<= 8;
		length |= part2;
		char privChar [50];
		sprintf(privChar,"length is : %d\n",length);
		OutputDebugString((LPCWSTR)privChar);
		if(length > 276){//255 + MSG_HEADER_LENGTH = 255 + 21
			sprintf(privChar,"length is too long");
			OutputDebugString((LPCWSTR)privChar);
			length =0;
			length = 0x00ff & part2;
			length <<= 8;
			length |= part1;
			oldMessage = 1;
		}

		//memcpy(&length,ptrBuffer,2);//get length of message

		

		receivedBytesMess = recv(client,ptrBuffer,length,0);
		if(receivedBytesMess == 0)
		{			
			netFSM->NetError();
			releaseAll();
		}
		else if(receivedBytesMess > 0)
		{
			//uspesno primljena poruka treba je obraditi za sada nije potrebna kriticna sekcija koristice kada vise klijenata upisuje u isti MBX
			//EnterCriticalSection(&CriticalSection);
			while(receivedBytesMess <= length-currentMsgLength)
			{
				memcpy(priv+currentMsgLength,ptrBuffer,receivedBytesMess);
				currentMsgLength += receivedBytesMess;

				if(length == currentMsgLength)
				{
					memcpy(ptrBuffer,priv,length);
					break;
				}


				receivedBytesMess = recv(client,ptrBuffer,length-currentMsgLength,0);//poslednji parametar je nasumicno postavljen ne treba mi ni jedna od ponudjenih vrednosti
				if(receivedBytesMess == 0)
				{			
					netFSM->NetError();
					releaseAll();
				}
				else if(receivedBytesMess < 0)
				{
					//doslo je do greske isfiltrirati koja je greska u pitanju
					//i u skladu sa time preuzeti dalje akcije
					getError();
					netFSM->NetError();
					releaseAll();
				}
			}

			if(!oldMessage){
				netFSM->receivedMessageLength = length;
			    memcpy(netFSM->protocolMessageR,ptrBuffer,length);
			    netFSM->currentMessageCode = netFSM->convertNetToFSMMessage();
		    	netFSM->workWhenReceive(netFSM->getProtocolInfoCoding());
			    //LeaveCriticalSection(&CriticalSection);
			}
		}
		else
		{
			//doslo je do greske isfiltrirati koja je greska u pitanju
			//i u skladu sa time preuzeti dalje akcije
			getError();
			netFSM->NetError();
			releaseAll();
		}
	}
	else
	{
		//doslo je do greske isfiltrirati koja je greska u pitanju
		//i u skladu sa time preuzeti dalje akcije
		getError();
		netFSM->NetError();
		releaseAll();
	}
}






