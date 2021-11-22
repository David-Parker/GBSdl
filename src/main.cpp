#include <iostream>
#include <string>
#include "GameBoy.h"
#include "SDLGraphicsHandler.h"
#include "SDLEventHandler.h"
#include "SDLSerialHandler.h"
#include "SDL.h"

#undef main

#define SCALE_4X 4.0

int main(int argc, char* argv[])
{
    std::string romPath;
    int listeningPort, clientPort;

    if (argc > 1)
    {
        romPath = argv[1];
    }
    else
    {
        std::cout << "Enter the name of your ROM file, e.g. Pokemon.gb" << std::endl;
        //std::cin >> romPath;
        //romPath = "Tetris.gb";
    }

    romPath = "Pokemon.gb";

    std::cout << "Enter the port to use for this GameBoy for serial connections." << std::endl;
    std::cin >> listeningPort;

    std::cout << "Enter the port of the other GameBoy for serial connections." << std::endl;
    std::cin >> clientPort;

    // Inject SDL based handlers for desktop builds.
    GameBoy* boy = new GameBoy(
        "./rom",
        new SDLGraphicsHandler(SCREEN_WIDTH, SCREEN_HEIGHT, SCALE_4X),
        new SDLEventHandler(),
        new SDLSerialHandler(listeningPort, clientPort));

    try
    {
        boy->LoadRom(romPath);
        boy->Start();
        boy->Run();
        boy->Stop();
        boy->SaveGame();
    }
    catch (std::exception& ex)
    {
        boy->Stop();
        std::cout << "Error detected. " << ex.what() << std::endl;
    }

    return 0;
}