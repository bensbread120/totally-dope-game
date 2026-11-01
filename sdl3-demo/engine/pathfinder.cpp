#include "pathfinder.h"
#include "gamestate.h"

void Pathfinder::initialisePath(const GameObject& obj) {
	Path ep = { obj.entity.id, 0, {} };
	paths.push_back(ep);
}

void Pathfinder::updatePaths(GameState& gs) {
	glm::vec2 playerPosition = gs.getPlayer().entity.position;
	glm::ivec2 playerCell = AStar::toGrid(playerPosition);

	targetLocation = playerPosition;

	AStar astar;
	astar.setBlockedCells(gs.walls);

	for (GameObject& character : gs.characters)
	{
		if (character.type == ObjectType::Player)
		{
			continue;
		}

		Path* path = findPath(character.entity.id);
		if (path == nullptr)
		{
			continue;
		}

		path->path = astar.findPath(AStar::toGrid(character.entity.position), playerCell);
		path->pathIndex = 0;
	}
}

Pathfinder::Path* Pathfinder::findPath(int id) {
	for (size_t i = 0; i < paths.size(); i++) {
		if (paths[i].entityIdent == id) {
			return &paths[i];
		}
	}

	return nullptr;
}