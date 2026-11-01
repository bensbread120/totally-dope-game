#pragma once

#include <set>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include "../../../gameobject.h"

class AStar {
public:
	using Cell = std::pair<int, int>;

	static constexpr int TILE_SIZE = 32;

	static glm::ivec2 toGrid(glm::vec2 worldPos);

	void setBlockedCells(const std::vector<GameObject>& walls); // Rebuild the blocked grid from wall positions
	// Directions from start to goal, empty if unreachable. avoid adds extra cells to treat as blocked for this search only
	std::vector<Direction> findPath(glm::ivec2 start, glm::ivec2 goal, const std::set<Cell>& avoid = {}) const;

private:
	// Flat grid covering the walls' bounding box, indexed by (y - origin.y) * cols + (x - origin.x).
	// Levels are enclosed by walls, so anything outside the box is treated as blocked
	glm::ivec2 origin{ 0 };
	int cols = 0;
	int rows = 0;
	std::vector<bool> blocked; // Grid cells that cannot be walked through

	bool inBounds(glm::ivec2 cell) const;
	int indexOf(glm::ivec2 cell) const;
	glm::ivec2 cellAt(int index) const;
	static Cell cellKey(glm::ivec2 cell);
	static int heuristic(glm::ivec2 a, glm::ivec2 b);
	static Direction directionBetween(glm::ivec2 from, glm::ivec2 to);
};
