#pragma once
#include <SDL3/SDL.h>
#include <algorithm>
#include <limits>
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

// Fraction (0..1) of `move` that box `a` can travel before it touches box `b`,
// so a mover can be stopped flush against something rather than inside it.
// Moving apart, or sliding along an edge, is never blocked.
inline float sweepContactTime(const SDL_FRect& a, glm::vec2 move, const SDL_FRect& b)
{
	const float aMin[2] = { a.x, a.y }, aMax[2] = { a.x + a.w, a.y + a.h };
	const float bMin[2] = { b.x, b.y }, bMax[2] = { b.x + b.w, b.y + b.h };
	const float m[2] = { move.x, move.y };

	// already overlapping (e.g. something walked into us): allow moving out, but not further in
	if (aMax[0] > bMin[0] && aMin[0] < bMax[0] && aMax[1] > bMin[1] && aMin[1] < bMax[1])
	{
		glm::vec2 apart = glm::vec2(a.x + a.w * 0.5f, a.y + a.h * 0.5f) - glm::vec2(b.x + b.w * 0.5f, b.y + b.h * 0.5f);
		return glm::dot(move, apart) >= 0.0f ? 1.0f : 0.0f;
	}

	float entry = -std::numeric_limits<float>::infinity();
	float exit = std::numeric_limits<float>::infinity();
	for (int axis = 0; axis < 2; axis++)
	{
		if (m[axis] == 0.0f)
		{
			// not moving on this axis, so the boxes must already overlap on it to ever touch
			if (aMax[axis] <= bMin[axis] || aMin[axis] >= bMax[axis]) return 1.0f;
			continue;
		}

		float t0 = ((m[axis] > 0.0f ? bMin[axis] - aMax[axis] : bMax[axis] - aMin[axis])) / m[axis];
		float t1 = ((m[axis] > 0.0f ? bMax[axis] - aMin[axis] : bMin[axis] - aMax[axis])) / m[axis];
		entry = std::max(entry, t0);
		exit = std::min(exit, t1);
	}

	if (entry >= exit || exit <= 0.0f || entry >= 1.0f)
	{
		return 1.0f; // never touches during this move, or already moving apart
	}
	return std::max(entry, 0.0f);
}

} // namespace engine
