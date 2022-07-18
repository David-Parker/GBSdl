#pragma once
#include "IEventHandler.h"

class SDLEventHandler :
    public IEventHandler
{
private:
    bool shouldQuit;
    int speedMultiplier;
    int baseMultiplier;
    int turboMultiplier;

public:
    SDLEventHandler(int baseMultiplier = 1, int turboMultiplier = 15)
        : shouldQuit(false), speedMultiplier(baseMultiplier), baseMultiplier(baseMultiplier), turboMultiplier(turboMultiplier)
    {
    }

    ~SDLEventHandler();

    void HandleInput(JoypadController* joypadController);
    bool ShouldQuit();
    int SpeedMultiplier();
};