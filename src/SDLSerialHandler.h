#pragma once
#include "ISerialHandler.h"
#include "SDL.h"
#include "SDL_net.h"
#include <thread>

class SDLSerialHandler :
    public ISerialHandler
{
private:
    TCPsocket listeningSocket;
    TCPsocket clientSocket;

    const char* clientIpAddress;
    int clientPort;
    bool enabled = true;
    bool recieveConnected = false;
    bool sendConnected = false;
    bool byteRecieved = false;
    Byte recievedByte;

    std::thread* recieveThread;
    std::thread* sendThread;

    void handleRecieve();
    void handleSend();

public:
    SDLSerialHandler(int listeningPort, int clientPort, const char* clientIpAddress, bool enabled = true);
    ~SDLSerialHandler();

    bool IsSerialConnected();
    void SendByte(Byte byte);
    bool ByteRecieved();
    Byte RecieveByte();
};