#pragma once
#include <SDL3/SDL.h>

namespace engine {

// Everything needed to open a window and get an SDL_Renderer going.
// Nothing here is specific to a platformer or a top-down game.
struct SDLState
{
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	int width = 0, height = 0, logW = 0, logH = 0;
	const bool* keys = nullptr;

	SDLState() : keys(SDL_GetKeyboardState(nullptr)) {}
};

inline bool initialise(SDLState& state, const char* windowTitle)
{
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL", nullptr);
		return false;
	}

	state.window = SDL_CreateWindow(windowTitle, state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
		return false;
	}

	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
		return false;
	}

	SDL_SetRenderVSync(state.renderer, 1);
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	return true;
}

inline void cleanup(SDLState& state)
{
	// The original version of this called SDL_DestroyRenderer/Window
	// unconditionally, including on the path where window creation failed
	// and `renderer` was never set -- these null checks close that off.
	if (state.renderer) SDL_DestroyRenderer(state.renderer);
	if (state.window) SDL_DestroyWindow(state.window);
	SDL_Quit();
}

} // namespace engine
