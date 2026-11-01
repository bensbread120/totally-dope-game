#pragma once

#include <set>
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
		glm::vec2 targetPosition; // Exact world position within the target cell, for the final approach off the grid
		bool needsUpdate; // Target changed, or route got blocked, since the path was last searched
		std::vector<Direction> path; // Array of directions to target
	};

	std::vector<Path> paths; // Array of path information, one per entity

	enum class Step {
		Walk,    // walk into the next cell of the path
		Hold,    // wait on the current cell, the way ahead is taken
		Arrived  // as close as the grid gets, approach targetPosition directly
	};

	static glm::ivec2 cellAt(glm::vec2 position); // Grid cell under the centre of a tile-sized sprite at position
	static glm::ivec2 cellOf(const GameObject& obj); // Grid cell under the object's centre

	void initialisePath(const GameObject& obj);
	void setBlockedCells(const std::vector<GameObject>& walls);
	void setTarget(glm::vec2 position, int id); // World position (sprite top-left) to head for
	void updatePath(int id); // Re-search the path if its target changed or its route was blocked
	Path* findPath(int id);

	// What the entity should do this frame. For Walk, cell is the next cell to walk into, otherwise the cell it holds
	Step nextStep(int id, const std::vector<GameObject>& characters, glm::ivec2& cell);
	void advance(int id); // Entity reached its next cell, move on to the following step

private:
	AStar search;

	bool peekNextCell(const Path& path, glm::ivec2& cell) const;
	std::set<AStar::Cell> reservedCells(int ignoreId) const; // Cells held or being walked into by other pathing entities
	bool isOccupied(glm::ivec2 cell, int ignoreId, const std::vector<GameObject>& characters, bool& byPathingEntity) const;
};
