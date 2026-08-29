#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>

#include <cstdlib>
#include <iostream>
#include <string>
#include "lib/Vector2.h"

int main(int argc, char* argv[]) {
    // Init SDL systems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ||
        IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG ||
        TTF_Init() == -1 ||
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Initialization error: " << SDL_GetError() << "\n";
        return 1;
    }

    int windowWidth = 800;
    int windowHeight = 600;

    // Create window + renderer
    SDL_Window* window = SDL_CreateWindow("SDL2 Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // // Load image texture
    // SDL_Texture* image = IMG_LoadTexture(renderer, "display/spaceship.png");  // PNG file in same folder
    // if (!image) {
    //     std::cerr << "Image load error: " << IMG_GetError() << "\n";
    // }
    
    // Load font and create a texture from text
    TTF_Font* font = TTF_OpenFont("display/OpenSans-Regular.ttf", 28); // Include a .ttf font
    SDL_Color white = {255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, "Hello SDL2 using vector2 22", white);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {50, 50, textSurface->w, textSurface->h};
    SDL_FreeSurface(textSurface);

    // Load sound
    Mix_Chunk* sound = Mix_LoadWAV("display/Movement2.wav"); // Include a short sound effect

    bool quit = false;
    SDL_Event event;

    Uint32 startTime = SDL_GetTicks(); // Timing
    
    enum Direction {
        UP,
        DOWN,
        LEFT,
        RIGHT,
    };

    Vector2 snakeLoc = Vector2(400, 500);
    Vector2 snakeLoc2 = Vector2(400, 510);
    std::vector<Vector2> snakeLocs;
    snakeLocs.push_back(snakeLoc);
    snakeLocs.push_back(snakeLoc2);
    Direction snakeDir = UP;
    int numSegs = 1;
    Vector2 pelletLoc = Vector2(rand() % windowWidth + 1, rand() % windowHeight + 1);

    while (!quit) {
        // Time tracking
        Uint32 elapsedTime = SDL_GetTicks() - startTime;
        // Event loop
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                quit = true;

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    Mix_PlayChannel(-1, sound, 0); // Play sound on spacebar
                } else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit = true; // quit when escape key is pressed.
                } else if (event.key.keysym.sym == SDLK_UP) {
                    snakeDir = UP;
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    snakeDir = DOWN;
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    snakeDir = RIGHT;
                } else if (event.key.keysym.sym == SDLK_LEFT) {
                    snakeDir = LEFT;
                }
            }
        }

        Vector2 firstSegLoc = snakeLocs.front();
        if (firstSegLoc.distance(pelletLoc) <= 10) {
            Vector2 newSeg = Vector2(firstSegLoc.x, firstSegLoc.y);
            snakeLocs.insert(snakeLocs.begin(), newSeg);
            pelletLoc = Vector2(rand() % windowWidth + 1, rand() % windowHeight + 1);
        }
        Vector2 lastSeg = snakeLocs.back();
        lastSeg.set(firstSegLoc);
        snakeLocs.pop_back();

        switch (snakeDir) {
            case UP:
                lastSeg.y = (firstSegLoc.y - 10) <= 0 ? windowHeight : firstSegLoc.y - 10;
                break;
            case DOWN:
                lastSeg.y = (firstSegLoc.y + 10) % windowHeight;
                break;
            case LEFT:
                lastSeg.x = firstSegLoc.x - 10 <= 0 ? windowWidth : firstSegLoc.x - 10;
                break;
            case RIGHT:
                lastSeg.x = (firstSegLoc.x + 10) % windowWidth;
                break;
        } 

        snakeLocs.insert(snakeLocs.begin(), lastSeg);

        

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
        SDL_RenderClear(renderer);

        // Draw filled rectangle
        for (int i = 0; i < snakeLocs.size(); i++) {
            SDL_Rect rect = {snakeLocs[i].x, snakeLocs[i].y, 10, 10};
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white
            SDL_RenderFillRect(renderer, &rect);
        }
        
        SDL_Rect rect = {pelletLoc.x, pelletLoc.y, 10, 10};
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); // purple
        SDL_RenderFillRect(renderer, &rect);

        // Draw text timer
        SDL_Surface* textSurface = TTF_RenderText_Blended(font, std::to_string(elapsedTime / 1000).c_str(), white);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        textRect = {50, 50, textSurface->w, textSurface->h};
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);


        // // Draw image
        // if (image) {
        //     SDL_Rect imgRect = {imageLoc.x, imageLoc.y, imageWidth, imageHeight};
        //     SDL_RenderCopy(renderer, image, NULL, &imgRect);
        // }


        // Present the frame
        SDL_RenderPresent(renderer);

        // Cap FPS (rudimentary)
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    Mix_FreeChunk(sound);
    SDL_DestroyTexture(textTexture);
    // SDL_DestroyTexture(image);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
