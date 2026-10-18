#include "pathfinder.h"
#include "gamestate.h"

void initialisePath(Pathfinder& pf, const GameObject& obj) {
	Path ep = { obj.entity.id, 0, {} };
	pf.paths.push_back(ep);
}

void updatePaths(GameState& gs) {
	if (gs.getPlayer().entity.position == gs.pathfinder.targetLocation) {
		return;
	}
	for (Path& path : gs.pathfinder.paths) {
		path.path.push_back(Direction::Down);
		path.path.push_back(Direction::Left);
	}
}