#pragma once
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <string>

namespace engine {

// Generic texture loading + cleanup. Nothing in here knows about
// idle/run/enemy/bullet -- that vocabulary is specific to each game's
// Resources struct, which should hold one of these instead of duplicating
// loadTexture()/unload().
class TextureCache
{
	std::vector<SDL_Texture*> textures;

public:
	// `filepath` is relative to the executable's folder (the build copies
	// assets/ there), not the current working directory -- so the game
	// finds its textures however it's launched.
	SDL_Texture* load(SDL_Renderer* renderer, const std::string& filepath,
	                   SDL_ScaleMode scaleMode = SDL_SCALEMODE_NEAREST)
	{
		const char* basePath = SDL_GetBasePath();
		std::string fullPath = basePath ? basePath + filepath : filepath;

		SDL_Log("Loading texture: %s", fullPath.c_str());
		SDL_Texture* tex = IMG_LoadTexture(renderer, fullPath.c_str());
		if (!tex)
		{
			SDL_Log("FAILED: %s", SDL_GetError());
			return nullptr;
		}
		SDL_SetTextureScaleMode(tex, scaleMode);
		textures.push_back(tex);
		return tex;
	}

	void unload()
	{
		for (SDL_Texture* tex : textures)
			SDL_DestroyTexture(tex);
		textures.clear();
	}

	~TextureCache() { unload(); }
};

} // namespace engine
