#include "SDLSerialHandler.h"
#include <Ws2tcpip.h>
#include <iostream>

SDLSerialHandler::SDLSerialHandler(int listeningPort, int clientPort, const char* clientIpAddress)
	: clientPort(clientPort), clientIpAddress(clientIpAddress)
{
	WORD sockVersion;
	WSADATA wsaData;
	int nret;

	/* Winsock version 2.2 */
	sockVersion = MAKEWORD(2, 2);

	/* Initialize Winsock */
	WSAStartup(sockVersion, &wsaData);

	/* Create a socket using TCP protocol */
	listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Disable Nagle's Algorithm*/
	int flags = 0; 
	setsockopt(listeningSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags));

	/* Address Information */
	SOCKADDR_IN serverInfo;

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = INADDR_ANY;
	serverInfo.sin_port = htons(listeningPort);

	/* Bind the socket to the port */
	nret = bind(listeningSocket, (LPSOCKADDR)&serverInfo, sizeof(struct sockaddr));

	/* Socket will now listen for a client */
	nret = listen(listeningSocket, 1);

	/* Server successfully established! */
	std::cout << "Server established!" << std::endl;

	this->recieveThread = new std::thread(&SDLSerialHandler::handleRecieve, this);
	this->sendThread = new std::thread(&SDLSerialHandler::handleSend, this);
}

SDLSerialHandler::~SDLSerialHandler()
{
	closesocket(listeningSocket);
	closesocket(clientSocket);
	WSACleanup();
	delete this->recieveThread;
	delete this->sendThread;
}

bool SDLSerialHandler::IsSerialConnected()
{
    return this->sendConnected && this->recieveConnected;
}

void SDLSerialHandler::SendByte(Byte byte)
{
	char buffer[2];
	buffer[0] = byte;
	buffer[1] = 0;

	char num[32];
	//std::cout << "Byte Sent: " << itoa(byte, num, 10) << std::endl;
	send(clientSocket, buffer, 1, 0);
}

bool SDLSerialHandler::ByteRecieved()
{
    return this->byteRecieved;
}

Byte SDLSerialHandler::RecieveByte()
{
	while (this->byteRecieved == false)
	{
		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	this->byteRecieved = false;

	char num[32];
	//std::cout << "Byte Recieved: " << itoa(recievedByte, num, 10) << std::endl;
    return this->recievedByte;
}

void SDLSerialHandler::Synchronize()
{

}

void SDLSerialHandler::handleRecieve()
{
	struct sockaddr_in clientAddr;
	int addrLen = sizeof(struct sockaddr_in);

	SOCKET client = accept(listeningSocket, NULL, NULL);

	if (client == INVALID_SOCKET)
	{
		printf("accept() failed with error code : %d\n", WSAGetLastError());
	}
	else
	{
		std::cout << "Recieve connection established." << std::endl;

		this->recieveConnected = true;

		char buffer[2];
		while (recv(client, buffer, 1, 0))
		{
			// Simulate 100ms of latency
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));
			this->recievedByte = (Byte)buffer[0];
			this->byteRecieved = true;
		}
	}
}

void SDLSerialHandler::handleSend()
{
	/* Store server information */
	in_addr iaHost;
	iaHost.s_addr = inet_addr(this->clientIpAddress);
	LPHOSTENT hostEntry = gethostbyaddr((const char*)&iaHost, sizeof(struct in_addr), AF_INET);

	/* Fill a SOCKADD_IN struct with address information */
	SOCKADDR_IN serverInfo;

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr = *((LPIN_ADDR)*hostEntry->h_addr_list);
	serverInfo.sin_port = htons(this->clientPort);

	this->clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Disable Nagle's Algorithm*/
	int flags = 0;
	setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flags, sizeof(flags));

	int result = SOCKET_ERROR;

	/* Connect to the server */
	while (result == SOCKET_ERROR)
	{
		result = connect(clientSocket, (LPSOCKADDR)&serverInfo, sizeof(SOCKADDR_IN));
	}

	if (clientSocket == INVALID_SOCKET)
	{
		printf("connect() failed with error code : %d\n", WSAGetLastError());
	}
	else
	{
		/* Successfully connected to the server! */
		std::cout << "Send connection established." << std::endl;

		this->sendConnected = true;
	}
}