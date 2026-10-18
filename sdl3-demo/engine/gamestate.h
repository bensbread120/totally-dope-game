#pragma once

#include <vector>
#include "camera.h"
#include "pathfinder.h"
#include "../gameobject.h"

struct GameState
{
	std::vector<GameObject> walls;      // collidable
	std::vector<TileSprite> floorTiles; // decorative only, never checked for collision
	std::vector<GameObject> characters; // player + enemies
	Pathfinder pathfinder;
	int playerIdx = -1;
	engine::Camera camera;
	bool debugMode = false;

	GameObject& getPlayer() { return characters[playerIdx]; }
};