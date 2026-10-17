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
	SDL_Texture* load(SDL_Renderer* renderer, const std::string& filepath,
	                   SDL_ScaleMode scaleMode = SDL_SCALEMODE_NEAREST)
	{
		SDL_Log("Loading texture: %s", filepath.c_str());
		SDL_Texture* tex = IMG_LoadTexture(renderer, filepath.c_str());
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
