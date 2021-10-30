#include <iostream>
#include <string>
#include "GameBoy.h"
#include "SDLGraphicsHandler.h"
#include "SDLEventHandler.h"
#include "SDL.h"

#undef main

int main(int argc, char* argv[])
{
    std::string romPath;

    if (argc > 1)
    {
        romPath = argv[1];
    }
    else
    {
        std::cout << "Enter the path to your ROM file, e.g. rom/Pokemon.gb" << std::endl;
        std::cin >> romPath;
    }

    // Inject SDL based handlers for desktop builds.
    GameBoy* boy = new GameBoy(
        "rom/saves",
        new SDLGraphicsHandler(SCALED_SCREEN_WIDTH, SCALED_SCREEN_HEIGHT), 
        new SDLEventHandler());

    try
    {
        u64 framesElapsed = 0;
        boy->LoadRom(romPath);
        boy->Start();

        while (!boy->ShouldStop())
        {
            boy->Step();

            // Another frame has elapsed.
            if (boy->FramesElapsed() > framesElapsed)
            {
                framesElapsed = boy->FramesElapsed();
                boy->SimulateFrameDelay();
            }
        }

        boy->Stop();
        //boy->SaveGame();
    }
    catch (std::exception& ex)
    {
        boy->Stop();
        std::cout << "Error detected. " << ex.what() << std::endl;
    }

    return 0;
}