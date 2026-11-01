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
	std::set<Cell> blocked; // Grid cells that cannot be walked through

	static Cell cellKey(glm::ivec2 cell);
	static int heuristic(glm::ivec2 a, glm::ivec2 b);
	static Direction directionBetween(glm::ivec2 from, glm::ivec2 to);
};
