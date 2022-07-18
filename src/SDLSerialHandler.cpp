#include "SDLSerialHandler.h"
#include <iostream>

SDLSerialHandler::SDLSerialHandler(int listeningPort, int clientPort, const char* clientIpAddress, bool enabled)
    : clientPort(clientPort), clientIpAddress(clientIpAddress), listeningSocket(nullptr), clientSocket(nullptr), enabled(enabled)
{
    if (!enabled)
    {
        return;
    }

    if (SDLNet_Init() == -1)
    {
        throw std::runtime_error("Failed to initialize SDL_net.");
    }

    IPaddress addr;
    SDLNet_ResolveHost(&addr, NULL, listeningPort);

    this->listeningSocket = SDLNet_TCP_Open(&addr);

    if (this->listeningSocket == NULL)
    {
        throw std::runtime_error("Failed to open listening server.");
    }

    // Server successfully established!
    std::cout << "Server established!" << std::endl;

    this->recieveThread = new std::thread(&SDLSerialHandler::handleRecieve, this);
    this->sendThread = new std::thread(&SDLSerialHandler::handleSend, this);
}

SDLSerialHandler::~SDLSerialHandler()
{
    delete this->recieveThread;
    delete this->sendThread;
    SDLNet_TCP_Close(this->listeningSocket);
    SDLNet_TCP_Close(this->clientSocket);
    SDLNet_Quit();
}

bool SDLSerialHandler::IsSerialConnected()
{
    if (!this->enabled)
    {
        return false;
    }

    return this->sendConnected && this->recieveConnected;
}

void SDLSerialHandler::SendByte(Byte byte)
{
    char buffer[2];
    buffer[0] = byte;
    buffer[1] = 0;

    printf("Byte Sent: 0x%02X\n", byte);
    SDLNet_TCP_Send(this->clientSocket, buffer, 2);
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

    printf("Byte Recieved: 0x%02X\n", recievedByte);
    this->byteRecieved = false;
    return this->recievedByte;
}

void SDLSerialHandler::handleRecieve()
{
    TCPsocket client = nullptr;
    char buffer[2];

    while (client == nullptr)
    {
        client = SDLNet_TCP_Accept(this->listeningSocket);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    this->recieveConnected = true;
    std::cout << "Recieve connection established." << std::endl;

    while (SDLNet_TCP_Recv(client, buffer, 2))
    {
        // Simulate 100ms of latency
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        this->recievedByte = (Byte)buffer[0];
        this->byteRecieved = true;
    }
}

void SDLSerialHandler::handleSend()
{
    IPaddress addr;
    SDLNet_ResolveHost(&addr, clientIpAddress, clientPort);

    while (this->clientSocket == nullptr)
    {
        this->clientSocket = SDLNet_TCP_Open(&addr);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    this->sendConnected = true;

    // Successfully connected to the server!
    std::cout << "Send connection established." << std::endl;
}