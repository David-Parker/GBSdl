#include <iostream>
#include <fstream>
#include <string>
#include "GameBoy.h"
#include "SDLGraphicsHandler.h"
#include "SDLEventHandler.h"
#include "SDLSerialHandler.h"
#include "SDL.h"
#include <json/json.h>

#undef main

#define SCALE_4X 4.0

int main(int argc, char* argv[])
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string romPath;
    std::ifstream jsonFile;
    jsonFile.open("connections.json", std::ios::in);

    if (!jsonFile.is_open())
    {
        throw std::runtime_error("connections.json file not found!.");
    }

    if (argc > 1)
    {
        romPath = argv[1];
    }
    else
    {
        std::cout << "Enter the name of your ROM file, e.g. Pokemon.gb" << std::endl;
        std::cin >> romPath;
    }

    JSONCPP_STRING errs;
    if (!parseFromStream(builder, jsonFile, &root, &errs)) 
    {
        std::cout << errs << std::endl;
        return EXIT_FAILURE;
    }

    // Inject SDL based handlers for desktop builds.
    GameBoy* boy = new GameBoy(
        "./rom",
        new SDLGraphicsHandler(SCREEN_WIDTH, SCREEN_HEIGHT, SCALE_4X),
        new SDLEventHandler(),
        new SDLSerialHandler(root["listeningPort"].asInt(), root["clientPort"].asInt(), root["clientIpAddress"].asCString()));

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

    // Wait until user closes the cmd window.
    std::cin >> romPath;

    return 0;
}