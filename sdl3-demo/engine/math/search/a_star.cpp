#include "a_star.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

glm::ivec2 AStar::toGrid(glm::vec2 worldPos)
{
	return glm::ivec2(
		static_cast<int>(std::floor(worldPos.x / TILE_SIZE)),
		static_cast<int>(std::floor(worldPos.y / TILE_SIZE)));
}

AStar::Cell AStar::cellKey(glm::ivec2 cell) { return { cell.x, cell.y }; }

int AStar::heuristic(glm::ivec2 a, glm::ivec2 b)
{
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Direction AStar::directionBetween(glm::ivec2 from, glm::ivec2 to)
{
	if (to.x > from.x) return Direction::Right;
	if (to.x < from.x) return Direction::Left;
	if (to.y > from.y) return Direction::Down;
	return Direction::Up;
}

void AStar::setBlockedCells(const std::vector<GameObject>& walls)
{
	blocked.clear();
	for (const GameObject& wall : walls)
	{
		blocked.insert(cellKey(toGrid(wall.entity.position)));
	}
}

std::vector<Direction> AStar::findPath(glm::ivec2 start, glm::ivec2 goal, const std::set<Cell>& avoid) const
{
	std::vector<Direction> result;
	if (start == goal)
	{
		return result;
	}

	static const glm::ivec2 neighbourOffsets[4] = {
		{ 0, -1 },{ 0, 1 },{ -1, 0 },{ 1, 0 } // Up, Down, Left, Right
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
			if (blocked.count(neighbourKey) || avoid.count(neighbourKey))
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
