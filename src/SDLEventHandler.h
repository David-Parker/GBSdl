#pragma once
#include "IEventHandler.h"

class SDLEventHandler :
    public IEventHandler
{
private:
    bool shouldQuit;
    int speedMultiplier;
public:
    SDLEventHandler()
        : shouldQuit(false), speedMultiplier(1)
    {
    }

    ~SDLEventHandler();

    void HandleInput(JoypadController* joypadController);
    bool ShouldQuit();
    int SpeedMultiplier();
};