#pragma once

#include <vector>
#include "camera.h"
#include "pathfinder.h"
#include "../gameobject.h"

struct GameState
{
	std::vector<GameObject> walls;      // collidable
	std::vector<int> wallGrid;          // index into walls for each grid cell (-1 = none), so collision only checks nearby walls
	glm::ivec2 wallGridOrigin{ 0 };     // grid cell of wallGrid[0]
	int wallGridCols = 0;
	int wallGridRows = 0;
	std::vector<TileSprite> floorTiles; // decorative only, never checked for collision
	std::vector<GameObject> characters; // player + enemies
	Pathfinder pathfinder;
	int playerIdx = -1;
	engine::Camera camera;
	bool debugMode = false;

	GameObject& getPlayer() { return characters[playerIdx]; }
};