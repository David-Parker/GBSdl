#pragma once
#include "IGraphicsHandler.h"
#include "SDL.h"

class SDLGraphicsHandler :
    public IGraphicsHandler
{
private:
    int width, height;
    float scale;
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Rect scaledTexture;

public:
    SDLGraphicsHandler(int width, int height, float scale);
    ~SDLGraphicsHandler();

    void Init();
    void Clear();
    void Draw(u32* pixelBuffer, int width, int height, int layer);
    void Flush();
    void Quit();
};