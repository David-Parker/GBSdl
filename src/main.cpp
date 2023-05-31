#include <iostream>
#include <fstream>
#include <string>
#include "EMUType.h"
#include "GameBoy.h"
#include "SDLGraphicsHandler.h"
#include "SDLEventHandler.h"
#include "SDLSerialHandler.h"
#include "SDL.h"
#include <json/json.h>

#undef main

int main(int argc, char* argv[])
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::ifstream jsonFile;
    jsonFile.open("settings.json", std::ios::in);

    if (!jsonFile.is_open())
    {
        throw std::runtime_error("settings.json file not found!.");
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
    EMUType emuType;

    if (root["emulator"]["forceDMG"].asBool() == true)
    {
        emuType = EMUType::DMG;
    }
    else if (root["emulator"]["forceCGB"].asBool() == true)
    {
        emuType = EMUType::CGB;
    }
    else 
    {
        emuType = EMUType::Cartridge;
    }

    // Inject SDL based handlers for desktop builds.
    GameBoy* boy = new GameBoy(
        root["rom"]["path"].asString(),
        new SDLGraphicsHandler(SCREEN_WIDTH, SCREEN_HEIGHT, root["emulator"]["screenScale"].asFloat()),
        new SDLEventHandler(root["emulator"]["baseMultiplier"].asInt(), root["emulator"]["turboMultiplier"].asInt()),
        new SDLSerialHandler(root["serialConnection"]["listeningPort"].asInt(), root["serialConnection"]["clientPort"].asInt(), root["serialConnection"]["clientIpAddress"].asCString(), root["serialConnection"]["enabled"].asBool()),
        emuType);

    try
    {
        boy->LoadRom(root["rom"]["title"].asString());
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