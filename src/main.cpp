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
    jsonFile.open("settings.json", std::ios::in);

    if (!jsonFile.is_open())
    {
        throw std::runtime_error("settings.json file not found!.");
    }

    // Read the command line args for the path to the ROM folder.
    if (argc > 1)
    {
        romPath = argv[1];
    }
    else
    {
        std::cout << "Enter the name of your ROM file, e.g. Pokemon.gb" << std::endl;
        std::cin >> romPath;
    }

    // Parse the settings.json file.
    JSONCPP_STRING errs;
    if (!parseFromStream(builder, jsonFile, &root, &errs)) 
    {
        std::cout << errs << std::endl;
        return EXIT_FAILURE;
    }

    // The settings allow for the user to force the emulator into either DMG or CGB mode.
    // Default is to use the value specified in the game cartridge.
    GameBoy::EMUType emuType;

    if (root["emulatorType"]["forceDMG"].asBool() == true)
    {
        emuType = GameBoy::EMUType::DMG;
    }
    else if (root["emulatorType"]["forceCGB"].asBool() == true)
    {
        emuType = GameBoy::EMUType::CGB;
    }
    else 
    {
        emuType = GameBoy::EMUType::Cartridge;
    }

    // Inject SDL based handlers for desktop builds.
    GameBoy* boy = new GameBoy(
        "./rom",
        new SDLGraphicsHandler(SCREEN_WIDTH, SCREEN_HEIGHT, SCALE_4X),
        new SDLEventHandler(),
        new SDLSerialHandler(root["serialConnection"]["listeningPort"].asInt(), root["serialConnection"]["clientPort"].asInt(), root["serialConnection"]["clientIpAddress"].asCString()),
        emuType);

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