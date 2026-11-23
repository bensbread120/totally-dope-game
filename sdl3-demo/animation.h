#pragma once
#include "timer.h"
#include "sound.h"
#include <vector>

class Animation
{
	int entityId = -1;
	const std::vector<SDL_Texture*>* textures = nullptr;
	int idx = 0;
	Timer timer;
	int frameCount;
	bool loop;

public:
	Sound sound;

	Animation() : timer(0), frameCount(0), loop(true), sound() {}

	// A one-shot animation (loop = false) plays once and then holds its last
	// frame, with isDone() reporting true, until reset() starts it again.
	Animation(int frameCount, float length, bool loop = true, Sound sound = {})
		: timer(length), frameCount(frameCount), loop(loop), sound(sound) {}

	
	// Switch to the clip for `anim` facing `dir`. `restart` rewinds it, which a
	// one-shot clip like an attack needs every time it's played.
	void play(bool restart = false)
	{
		if (restart)
		{
			reset();
		}
		if (sound.startFrame == currentFrame()) {
			playSound(sound);
		}
	}

	float getLength() const {
		return timer.getLength();
	}

	int getFrameCount() const {
		return frameCount;
	}

	int currentFrame() const
	{
		if (!loop && timer.isTimeout())
		{
			return frameCount - 1;
		}
		int frame = static_cast<int>((timer.getTime() / timer.getLength()) * frameCount);
		return frame < frameCount ? frame : frameCount - 1;
	}

	void step(float deltaTime) {
		if (!loop && timer.isTimeout())
		{
			return;
		}
		timer.step(deltaTime);
	}

	void reset() {
		timer.reset();
	}

	bool isDone() const { return timer.isTimeout(); }
};
