#pragma once
#include "ISerialHandler.h"
#include "SDL.h"
#include <winsock2.h>
#include <thread>

class SDLSerialHandler :
    public ISerialHandler
{
private:
    SOCKET listeningSocket;
    SOCKET clientSocket;

    int clientPort;
    bool recieveConnected = false;
    bool sendConnected = false;
    bool byteRecieved = false;
    Byte recievedByte;

    std::thread* recieveThread;
    std::thread* sendThread;

    void handleRecieve();
    void handleSend();

public:
    SDLSerialHandler(int listeningPort, int clientPort);
    ~SDLSerialHandler();

    bool IsSerialConnected();
    void SendByte(Byte byte);
    bool ByteRecieved();
    Byte RecieveByte();
};