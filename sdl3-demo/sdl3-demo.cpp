// sdl3-demo.cpp : Defines the entry point for the application.
//
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "sdl3-demo.h"

using namespace std;

struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
};

bool initialise(SDLState& state);
void cleanup(SDLState &state);

int main(int argc, char* argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;
	if (!initialise(state)) {
		return 1;
	}

	// load game assets
	SDL_Texture* idleTex = IMG_LoadTexture(state.renderer, "assets/idle.png");
	SDL_SetTextureScaleMode(idleTex, SDL_SCALEMODE_NEAREST);

	const bool* keys = SDL_GetKeyboardState(nullptr);
	float playerX = 0.0f;
	const float floor = state.logH;
	// Game loop begin
	bool running = true;
	while (running) {
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT: {
					running = false;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED: {
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
				}
			}
		}

		// handle movement
		float moveAmount = 0;
		if (keys[SDL_SCANCODE_LEFT]) {
			moveAmount -= 1.0f;
		}
		if (keys[SDL_SCANCODE_RIGHT]) {
			moveAmount += 1.0f;
		}
		playerX += moveAmount;

		// perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 55, 55, 255);
		SDL_RenderClear(state.renderer);

		const float spriteSize = 32;
		SDL_FRect src{
			.x = 0,
			.y = 0,
			.w = spriteSize,
			.h = spriteSize
		};

		SDL_FRect dst{
			.x = playerX,
			.y = floor - spriteSize,
			.w = spriteSize,
			.h = spriteSize
		};

		SDL_RenderTexture(state.renderer, idleTex, &src, &dst);

		// swap buffer and present
		SDL_RenderPresent(state.renderer);

	}

	SDL_DestroyTexture(idleTex);
	cleanup(state);
	return 0;
}

bool initialise(SDLState& state) {
	bool initSuccess = true;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL", nullptr);
		initSuccess = false;
	}

	// Create window
	state.window = SDL_CreateWindow("SDL3 demo", state.width, state.height, SDL_WINDOW_RESIZABLE);

	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	// create window renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	// configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	
	return initSuccess;
}

void cleanup(SDLState &state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}