#pragma once
#include <glm/glm.hpp>
#include <SDL3/SDL.h>

namespace engine {

// A viewport that centers on a target. Your platformer only ever follows X
// (the level is one screen tall), a top-down game will follow both axes --
// same struct, just call centerOn with different follow flags.
struct Camera
{
	SDL_FRect viewport{ 0, 0, 0, 0 };

	void centerOn(const glm::vec2& targetPos, float targetWidth, float targetHeight,
	              bool followX = true, bool followY = true)
	{
		if (followX) viewport.x = (targetPos.x + targetWidth / 2) - viewport.w / 2;
		if (followY) viewport.y = (targetPos.y + targetHeight / 2) - viewport.h / 2;
	}
};

} // namespace engine
