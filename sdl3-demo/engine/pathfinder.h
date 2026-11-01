#pragma once

#include <vector>
#include "../gameobject.h"
#include "math/search/a_star.h"

struct GameState;

class Pathfinder {
public:
	struct Path {
		int entityIdent; // Entity this path belongs to
		int pathIndex; // Current index within path
		std::vector<Direction> path; // Array of directions to target
	};

	glm::vec2 targetLocation; // Target location, allows checking if target moves or have entities switch targets
	std::vector<Path> paths; // Array of path information, one per entity
	
	void initialisePath(const GameObject& obj);
	void updatePaths(GameState& gs);
	Path* findPath(int id);
};
