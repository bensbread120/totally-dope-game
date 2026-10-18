#pragma once
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <vector>
#include <atomic>
#include "../animation.h"

namespace engine {

// Everything a moving, drawable, collidable thing needs, regardless of
// whether "moving" means running/jumping along one axis (platformer) or
// walking in eight directions (top-down). Nothing in here knows what a
// player, an enemy, or a wall is -- that vocabulary belongs to each game.
struct Entity
{
	inline static std::atomic<int> next_id{ 1 };
	
	int id = next_id++;
	glm::vec2 position{ 0.0f };
	glm::vec2 velocity{ 0.0f };
	glm::vec2 acceleration{ 0.0f };
	glm::vec2 maxSpeed{ 0.0f };     // per-axis cap; the platformer only ever sets .x

	// Last non-zero movement direction. The platformer only reads facing.x
	// (sprite flip + bullet spawn offset); a top-down game will want both
	// axes for 4/8-directional sprites and attack aiming.
	glm::vec2 facing{ 1.0f, 0.0f };

	SDL_Texture* texture = nullptr;
	std::vector<Animation> animations;
	int currentAnimation = -1;

	SDL_FRect collider{ 0, 0, 0, 0 }; // offset + size, relative to position
	bool dynamic = false;             // participates in physics/collision

	SDL_FRect worldCollider() const
	{
		return SDL_FRect{
			position.x + collider.x, position.y + collider.y,
			collider.w, collider.h
		};
	}

	void stepAnimation(float deltaTime)
	{
		if (currentAnimation != -1)
			animations[currentAnimation].step(deltaTime);
	}
};

} // namespace engine
