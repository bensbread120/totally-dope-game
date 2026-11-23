#pragma once
#include <SDL3/SDL.h>

struct Sound
{
	SDL_AudioStream* audiostream = nullptr;
	Uint8* wavData = nullptr;
	Uint32 wavDataLen = 0;
	Uint8 startFrame = 0;
};

inline void playSound(Sound sound)
{
	SDL_PutAudioStreamData(sound.audiostream, sound.wavData, (int)sound.wavDataLen);
}