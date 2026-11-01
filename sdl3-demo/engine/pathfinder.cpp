#include "pathfinder.h"

void Pathfinder::initialisePath(const GameObject& obj) {
	// start targeting our own cell so the entity stays put until given a target
	glm::ivec2 cell = AStar::toGrid(obj.entity.position);
	Path ep = { obj.entity.id, 0, cell, cell, false, {} };
	paths.push_back(ep);
}

void Pathfinder::setBlockedCells(const std::vector<GameObject>& walls) {
	search.setBlockedCells(walls);
}

void Pathfinder::setTarget(glm::ivec2 target, int id) {
	Path* path = findPath(id);
	if (path == nullptr || path->target == target)
	{
		return;
	}

	path->target = target;
	path->needsUpdate = true;
}

void Pathfinder::updatePath(const GameObject& obj) {
	Path* path = findPath(obj.entity.id);
	if (path == nullptr || !path->needsUpdate)
	{
		return;
	}

	path->cell = AStar::toGrid(obj.entity.position);
	path->path = search.findPath(path->cell, path->target);
	path->pathIndex = 0;
	path->needsUpdate = false;
}

bool Pathfinder::nextCell(int id, glm::ivec2& cell) {
	Path* path = findPath(id);
	if (path == nullptr || path->pathIndex >= static_cast<int>(path->path.size()))
	{
		return false;
	}

	cell = path->cell + glm::ivec2(directionVector(path->path[path->pathIndex]));
	return true;
}

void Pathfinder::advance(int id) {
	glm::ivec2 cell;
	if (!nextCell(id, cell))
	{
		return;
	}

	Path* path = findPath(id);
	path->cell = cell;
	path->pathIndex += 1;
}

Pathfinder::Path* Pathfinder::findPath(int id) {
	for (size_t i = 0; i < paths.size(); i++) {
		if (paths[i].entityIdent == id) {
			return &paths[i];
		}
	}

	return nullptr;
}
