#pragma once
#include <SDL3/SDL.h>
#include "entity.h"

namespace engine {

// Pure AABB overlap test in world space. Has no idea what a "platformer" or
// "top-down" game is -- identical in both.
inline bool checkOverlap(const Entity& a, const Entity& b, SDL_FRect& outOverlap)
{
	SDL_FRect rectA = a.worldCollider();
	SDL_FRect rectB = b.worldCollider();
	return SDL_GetRectIntersectionFloat(&rectA, &rectB, &outOverlap);
}

// Pushes `subject` out along the axis of least penetration and zeroes
// velocity on that axis. This "minimum translation vector" push-out is the
// same math whether you're landing on a platform or bumping into a rock in
// a top-down room -- your original genericResponse() lambda was already
// this generic, it was just trapped inside a bigger, platformer-only
// function alongside gravity and bullet-hit logic.
inline void resolveAxisAligned(Entity& subject, const SDL_FRect& overlap)
{
	if (overlap.w < overlap.h)
	{
		// shallower on X -> horizontal collision
		if (subject.velocity.x > 0) subject.position.x -= overlap.w;
		else if (subject.velocity.x < 0) subject.position.x += overlap.w;
		subject.velocity.x = 0;
	}
	else
	{
		// shallower on Y -> vertical collision
		if (subject.velocity.y > 0) subject.position.y -= overlap.h;
		else if (subject.velocity.y < 0) subject.position.y += overlap.h;
		subject.velocity.y = 0;
	}
}

} // namespace engine
