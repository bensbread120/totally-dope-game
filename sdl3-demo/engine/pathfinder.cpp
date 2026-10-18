#include "pathfinder.h"
#include "gamestate.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

const int TILE_SIZE = 32;

namespace {

using Cell = std::pair<int, int>;

Cell cellKey(glm::ivec2 cell) { return { cell.x, cell.y }; }

glm::ivec2 toGrid(glm::vec2 worldPos)
{
	return glm::ivec2(
		static_cast<int>(std::floor(worldPos.x / TILE_SIZE)),
		static_cast<int>(std::floor(worldPos.y / TILE_SIZE)));
}

int heuristic(glm::ivec2 a, glm::ivec2 b)
{
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Direction directionBetween(glm::ivec2 from, glm::ivec2 to)
{
	if (to.x > from.x) return Direction::Right;
	if (to.x < from.x) return Direction::Left;
	if (to.y > from.y) return Direction::Down;
	return Direction::Up;
}

std::set<Cell> collectBlockedCells(const std::vector<GameObject>& walls)
{
	std::set<Cell> blocked;
	for (const GameObject& wall : walls)
	{
		blocked.insert(cellKey(toGrid(wall.entity.position)));
	}
	return blocked;
}

std::vector<Direction> findAStarPath(glm::ivec2 start, glm::ivec2 goal, const std::set<Cell>& blocked)
{
	std::vector<Direction> result;
	if (start == goal)
	{
		return result;
	}

	static const glm::ivec2 neighbourOffsets[4] = {
		{0, -1}, {0, 1}, {-1, 0}, {1, 0} // Up, Down, Left, Right
	};

	std::set<Cell> openSet{ cellKey(start) };
	std::map<Cell, Cell> cameFrom;
	std::map<Cell, int> gScore{ { cellKey(start), 0 } };
	Cell goalKey = cellKey(goal);

	while (!openSet.empty())
	{
		// pick the open cell with the lowest f-score = g + heuristic
		Cell current = *openSet.begin();
		int bestF = std::numeric_limits<int>::max();
		for (const Cell& candidate : openSet)
		{
			int f = gScore[candidate] + heuristic({ candidate.first, candidate.second }, goal);
			if (f < bestF)
			{
				bestF = f;
				current = candidate;
			}
		}
		openSet.erase(current);

		if (current == goalKey)
		{
			// walk parents back to the start, then reverse into start->goal order
			std::vector<glm::ivec2> cells{ { current.first, current.second } };
			Cell step = current;
			while (cameFrom.count(step))
			{
				step = cameFrom[step];
				cells.push_back({ step.first, step.second });
			}
			std::reverse(cells.begin(), cells.end());

			for (size_t i = 0; i + 1 < cells.size(); i++)
			{
				result.push_back(directionBetween(cells[i], cells[i + 1]));
			}
			return result;
		}

		glm::ivec2 currentCell{ current.first, current.second };
		for (const glm::ivec2& offset : neighbourOffsets)
		{
			Cell neighbourKey = cellKey(currentCell + offset);
			if (blocked.count(neighbourKey))
			{
				continue;
			}

			int tentativeG = gScore[current] + 1;
			if (!gScore.count(neighbourKey) || tentativeG < gScore[neighbourKey])
			{
				cameFrom[neighbourKey] = current;
				gScore[neighbourKey] = tentativeG;
				openSet.insert(neighbourKey);
			}
		}
	}

	return result;
}

} // namespace

void initialisePath(Pathfinder& pf, const GameObject& obj) {
	Path ep = { obj.entity.id, 0, {} };
	pf.paths.push_back(ep);
}

void updatePaths(GameState& gs) {
	glm::vec2 playerPosition = gs.getPlayer().entity.position;
	glm::ivec2 playerCell = toGrid(playerPosition);

	if (playerCell == toGrid(gs.pathfinder.targetLocation))
	{
		return;
	}
	gs.pathfinder.targetLocation = playerPosition;

	std::set<Cell> blocked = collectBlockedCells(gs.walls);

	for (GameObject& character : gs.characters)
	{
		if (character.type == ObjectType::Player)
		{
			continue;
		}

		Path* path = findPath(gs.pathfinder, character.entity.id);
		if (path == nullptr)
		{
			continue;
		}

		path->path = findAStarPath(toGrid(character.entity.position), playerCell, blocked);
		path->pathIndex = 0;
	}
}

Path* findPath(Pathfinder& pf, int id) {
	for (size_t i = 0; i < pf.paths.size(); i++) {
		if (pf.paths[i].entityIdent == id) {
			return &pf.paths[i];
		}
	}

	return nullptr;
}