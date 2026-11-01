#pragma once

#include <vector>
#include "../gameobject.h"
#include "math/search/a_star.h"

class Pathfinder {
public:
	struct Path {
		int entityIdent; // Entity this path belongs to
		int pathIndex; // Current index within path
		glm::ivec2 cell; // Grid cell reached so far; path[pathIndex] leads out of it
		glm::ivec2 target; // Grid cell this entity is heading for
		bool needsUpdate; // Target changed since the path was last searched
		std::vector<Direction> path; // Array of directions to target
	};

	std::vector<Path> paths; // Array of path information, one per entity

	void initialisePath(const GameObject& obj);
	void setBlockedCells(const std::vector<GameObject>& walls);
	void setTarget(glm::ivec2 target, int id);
	void updatePath(const GameObject& obj); // Re-search obj's path if its target changed
	Path* findPath(int id);
	bool nextCell(int id, glm::ivec2& cell); // Cell the entity should walk into next, false if no steps remain
	void advance(int id); // Entity reached its next cell, move on to the following step

private:
	AStar search; // Search algorithm used to build paths
};
