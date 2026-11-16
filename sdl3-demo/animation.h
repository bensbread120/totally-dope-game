#pragma once
#include "timer.h"

class Animation
{
	Timer timer;
	int frameCount;
	bool loop;

public:
	Animation() : timer(0), frameCount(0), loop(true) {}

	// A one-shot animation (loop = false) plays once and then holds its last
	// frame, with isDone() reporting true, until reset() starts it again.
	Animation(int frameCount, float length, bool loop = true)
		: timer(length), frameCount(frameCount), loop(loop) {}

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
