#include "a_star.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>
#include <queue>

glm::ivec2 AStar::toGrid(glm::vec2 worldPos)
{
	return glm::ivec2(
		static_cast<int>(std::floor(worldPos.x / TILE_SIZE)),
		static_cast<int>(std::floor(worldPos.y / TILE_SIZE)));
}

bool AStar::inBounds(glm::ivec2 cell) const
{
	return cell.x >= origin.x && cell.x < origin.x + cols && cell.y >= origin.y && cell.y < origin.y + rows;
}

int AStar::indexOf(glm::ivec2 cell) const
{
	return (cell.y - origin.y) * cols + (cell.x - origin.x);
}

glm::ivec2 AStar::cellAt(int index) const
{
	return origin + glm::ivec2(index % cols, index / cols);
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
	cols = rows = 0;
	if (walls.empty())
	{
		return;
	}

	// size the grid to the walls' bounding box
	glm::ivec2 minCell = toGrid(walls[0].entity.position);
	glm::ivec2 maxCell = minCell;
	for (const GameObject& wall : walls)
	{
		glm::ivec2 cell = toGrid(wall.entity.position);
		minCell = glm::min(minCell, cell);
		maxCell = glm::max(maxCell, cell);
	}
	origin = minCell;
	cols = maxCell.x - minCell.x + 1;
	rows = maxCell.y - minCell.y + 1;

	blocked.assign(cols * rows, false);
	for (const GameObject& wall : walls)
	{
		blocked[indexOf(toGrid(wall.entity.position))] = true;
	}
}

std::vector<Direction> AStar::findPath(glm::ivec2 start, glm::ivec2 goal, const std::set<Cell>& avoid) const
{
	std::vector<Direction> result;
	if (start == goal || !inBounds(start) || !inBounds(goal))
	{
		return result;
	}

	static const glm::ivec2 neighbourOffsets[4] = {
		{ 0, -1 },{ 0, 1 },{ -1, 0 },{ 1, 0 } // Up, Down, Left, Right
	};

	// per-cell search state, indexed the same way as blocked
	const int cellCount = cols * rows;
	std::vector<int> gScore(cellCount, std::numeric_limits<int>::max());
	std::vector<int> cameFrom(cellCount, -1);
	std::vector<bool> closed(cellCount, false);

	// min-heap of (f-score, cell index). A cell can be pushed again when a cheaper
	// route to it turns up, so stale entries are skipped when popped instead of removed
	using Entry = std::pair<int, int>;
	std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> openSet;

	const int startIndex = indexOf(start);
	const int goalIndex = indexOf(goal);
	gScore[startIndex] = 0;
	openSet.push({ heuristic(start, goal), startIndex });

	while (!openSet.empty())
	{
		int current = openSet.top().second;
		openSet.pop();
		if (closed[current])
		{
			continue;
		}
		closed[current] = true;

		if (current == goalIndex)
		{
			// walk parents back to the start, then reverse into start->goal order
			std::vector<glm::ivec2> cells{ cellAt(current) };
			for (int step = cameFrom[current]; step != -1; step = cameFrom[step])
			{
				cells.push_back(cellAt(step));
			}
			std::reverse(cells.begin(), cells.end());

			for (size_t i = 0; i + 1 < cells.size(); i++)
			{
				result.push_back(directionBetween(cells[i], cells[i + 1]));
			}
			return result;
		}

		glm::ivec2 currentCell = cellAt(current);
		for (const glm::ivec2& offset : neighbourOffsets)
		{
			glm::ivec2 neighbourCell = currentCell + offset;
			if (!inBounds(neighbourCell))
			{
				continue;
			}

			int neighbour = indexOf(neighbourCell);
			if (closed[neighbour] || blocked[neighbour] || avoid.count(cellKey(neighbourCell)))
			{
				continue;
			}

			int tentativeG = gScore[current] + 1;
			if (tentativeG < gScore[neighbour])
			{
				cameFrom[neighbour] = current;
				gScore[neighbour] = tentativeG;
				openSet.push({ tentativeG + heuristic(neighbourCell, goal), neighbour });
			}
		}
	}

	return result;
}
