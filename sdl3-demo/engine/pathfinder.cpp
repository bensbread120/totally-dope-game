#include "pathfinder.h"

glm::ivec2 Pathfinder::cellAt(glm::vec2 position) {
	// entity positions are the sprite's top-left corner, so measure from the centre
	// of the tile-sized sprite to get the cell it is mostly standing in
	return AStar::toGrid(position + glm::vec2(AStar::TILE_SIZE * 0.5f));
}

glm::ivec2 Pathfinder::cellOf(const GameObject& obj) {
	return cellAt(obj.entity.position);
}

void Pathfinder::initialisePath(const GameObject& obj) {
	// start targeting our own cell so the entity stays put until given a target
	glm::ivec2 cell = cellOf(obj);
	Path ep = { obj.entity.id, 0, cell, cell, obj.entity.position, false, {} };
	paths.push_back(ep);
}

void Pathfinder::setBlockedCells(const std::vector<GameObject>& walls) {
	search.setBlockedCells(walls);
}

void Pathfinder::setTarget(glm::vec2 position, int id) {
	Path* path = findPath(id);
	if (path == nullptr)
	{
		return;
	}

	// the exact position can change freely; only a new cell needs a new search
	path->targetPosition = position;
	glm::ivec2 target = cellAt(position);
	if (path->target == target)
	{
		return;
	}

	path->target = target;
	path->needsUpdate = true;
}

void Pathfinder::updatePath(int id) {
	Path* path = findPath(id);
	if (path == nullptr || !path->needsUpdate)
	{
		return;
	}

	// route around other pathing entities, but never rule out the target itself,
	// otherwise a target someone is already standing on would be unreachable
	std::set<AStar::Cell> avoid = reservedCells(id);
	avoid.erase({ path->target.x, path->target.y });

	// search from the last cell we landed on, which we still hold, rather than our
	// exact position, which may be part way into a neighbouring cell
	path->path = search.findPath(path->cell, path->target, avoid);
	path->pathIndex = 0;

	// keep retrying while there is no way through, as the blocking entities may move
	path->needsUpdate = path->path.empty() && path->cell != path->target;
}

Pathfinder::Step Pathfinder::nextStep(int id, const std::vector<GameObject>& characters, glm::ivec2& cell) {
	Path* path = findPath(id);
	if (path == nullptr)
	{
		return Step::Hold;
	}

	cell = path->cell;
	glm::ivec2 next;
	if (!peekNextCell(*path, next))
	{
		// finished the path, unless there was no route at all and we're still waiting for one
		return path->needsUpdate ? Step::Hold : Step::Arrived;
	}

	bool byPathingEntity = false;
	if (isOccupied(next, id, characters, byPathingEntity))
	{
		// another pathing entity in the way of our route: wait and find a way around it
		if (byPathingEntity && next != path->target)
		{
			path->needsUpdate = true;
			return Step::Hold;
		}

		// someone on (or straddling into) the target cell: we're as close as the grid
		// gets. Anything else in the way further out, just wait for it to move
		glm::ivec2 offset = glm::abs(next - path->target);
		return offset.x + offset.y <= 1 ? Step::Arrived : Step::Hold;
	}

	cell = next;
	return Step::Walk;
}

void Pathfinder::advance(int id) {
	Path* path = findPath(id);
	glm::ivec2 next;
	if (path == nullptr || !peekNextCell(*path, next))
	{
		return;
	}

	path->cell = next;
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

bool Pathfinder::peekNextCell(const Path& path, glm::ivec2& cell) const {
	if (path.pathIndex >= static_cast<int>(path.path.size()))
	{
		return false;
	}

	cell = path.cell + glm::ivec2(directionVector(path.path[path.pathIndex]));
	return true;
}

std::set<AStar::Cell> Pathfinder::reservedCells(int ignoreId) const {
	std::set<AStar::Cell> reserved;
	for (const Path& other : paths)
	{
		if (other.entityIdent == ignoreId)
		{
			continue;
		}

		reserved.insert({ other.cell.x, other.cell.y });
		glm::ivec2 next;
		if (peekNextCell(other, next))
		{
			reserved.insert({ next.x, next.y });
		}
	}
	return reserved;
}

bool Pathfinder::isOccupied(glm::ivec2 cell, int ignoreId, const std::vector<GameObject>& characters, bool& byPathingEntity) const {
	byPathingEntity = reservedCells(ignoreId).count({ cell.x, cell.y }) > 0;
	if (byPathingEntity)
	{
		return true;
	}

	// characters without a path (e.g. the player) don't reserve cells, so check whether their body overlaps the cell
	SDL_FRect cellRect{
		static_cast<float>(cell.x * AStar::TILE_SIZE), static_cast<float>(cell.y * AStar::TILE_SIZE),
		static_cast<float>(AStar::TILE_SIZE), static_cast<float>(AStar::TILE_SIZE)
	};
	for (const GameObject& character : characters)
	{
		if (character.entity.id == ignoreId)
		{
			continue;
		}

		SDL_FRect body = character.entity.worldCollider();
		if (SDL_HasRectIntersectionFloat(&cellRect, &body))
		{
			return true;
		}
	}
	return false;
}
