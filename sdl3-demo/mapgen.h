#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <utility>
#include <algorithm>

// Random level layouts for the top-down game. The output is the same kind
// of tile grid createMap() used to hand-write, so the code that turns tiles
// into walls/floors/characters doesn't care where the grid came from.
//
// How a level is built:
//   1. The map is split into a lattice of square "slots". Starting from a
//      random slot, rooms grow outwards one neighbouring slot at a time.
//      Which slots end up used is what gives each level its overall shape.
//   2. Every used slot gets a room: either a rectangle with a couple of
//      extra rectangles bolted on, or an ellipse.
//   3. Rooms that grew from each other are joined by straight corridors
//      exactly two tiles wide, plus a few extra corridors to make loops.
//   4. Every empty tile touching the floor (diagonals included) becomes a
//      wall. Everything further out stays empty, so nothing is spent
//      drawing or colliding against solid rock nobody can reach.

enum MapTile : short
{
	TILE_FLOOR = 0,  // walkable
	TILE_WALL = 1,   // blocks movement
	TILE_PLAYER = 2, // player start (floor underneath)
	TILE_ENEMY = 3,  // enemy start (floor underneath)
	TILE_EMPTY = 4,  // outside the level, nothing placed
};

struct MapGenSettings
{
	int slotCols = 5;
	int slotRows = 5;
	int slotSize = 14;        // tiles per slot each way; keep it even and at least 10
	int minRooms = 8;
	int maxRooms = 13;
	float loopChance = 0.2f;  // chance of an extra corridor between two neighbouring rooms
	float ellipseChance = 0.25f;
	int maxEnemiesPerRoom = 2;
};

struct GeneratedMap
{
	int rows = 0;
	int cols = 0;
	Uint64 seed = 0;
	std::vector<short> cells; // row-major, rows * cols

	short& at(int r, int c) { return cells[r * cols + c]; }
	short at(int r, int c) const { return cells[r * cols + c]; }
	bool inBounds(int r, int c) const { return r >= 0 && r < rows && c >= 0 && c < cols; }
};

namespace mapgen_detail {

struct Rect { int x, y, w, h; }; // in tiles: x = column, y = row

// Own generator state rather than the global SDL_rand, so a seed always
// reproduces the same level no matter what else has rolled dice.
struct Rng
{
	Uint64 state;

	int range(int lo, int hi) { return lo + SDL_rand_r(&state, hi - lo + 1); } // inclusive
	bool chance(float p) { return SDL_randf_r(&state) < p; }
};

inline Rect intersect(const Rect& a, const Rect& b)
{
	int x0 = std::max(a.x, b.x), y0 = std::max(a.y, b.y);
	int x1 = std::min(a.x + a.w, b.x + b.w), y1 = std::min(a.y + a.h, b.y + b.h);
	return Rect{ x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0) };
}

inline void carve(GeneratedMap& map, const Rect& r)
{
	for (int y = r.y; y < r.y + r.h; y++)
		for (int x = r.x; x < r.x + r.w; x++)
			map.at(y, x) = TILE_FLOOR;
}

// Every room returns its "core": a rectangle that is guaranteed to be floor
// and covers the 2x2 block in the middle of its slot. Corridors are dug
// between cores, and because two rooms in the same lattice row (or column)
// share that middle block's rows (or columns), their cores always overlap
// by at least two tiles on the axis a corridor needs.

inline Rect carveBlockRoom(GeneratedMap& map, Rng& rng, const Rect& area, int cx, int cy)
{
	// a base rectangle anywhere in the slot, as long as it covers the
	// centre block (columns cx-1..cx, rows cy-1..cy)
	int w = rng.range(4, area.w);
	int h = rng.range(4, area.h);
	Rect base{
		rng.range(std::max(area.x, cx - w + 1), std::min(cx - 1, area.x + area.w - w)),
		rng.range(std::max(area.y, cy - h + 1), std::min(cy - 1, area.y + area.h - h)),
		w, h
	};
	carve(map, base);

	// up to two extra rectangles, each overlapping the base by at least a
	// 2x2 block so the join is never a one-tile pinch, giving L, T and
	// lumpier shapes instead of plain boxes
	int extras = rng.range(0, 2);
	for (int i = 0; i < extras; i++)
	{
		int ax = rng.range(base.x, base.x + base.w - 2);
		int ay = rng.range(base.y, base.y + base.h - 2);
		int ew = rng.range(3, area.w / 2 + 1);
		int eh = rng.range(3, area.h / 2 + 1);
		Rect extra{ rng.range(ax - (ew - 2), ax), rng.range(ay - (eh - 2), ay), ew, eh };
		carve(map, intersect(extra, area));
	}
	return base;
}

inline Rect carveEllipseRoom(GeneratedMap& map, Rng& rng, const Rect& area, int cx, int cy)
{
	// centred on the line between the two centre columns/rows, so each row
	// of the ellipse is an even number of tiles wide and symmetric
	int rx = rng.range(3, std::min(cx - area.x, area.x + area.w - cx));
	int ry = rng.range(3, std::min(cy - area.y, area.y + area.h - cy));
	for (int y = cy - ry; y < cy + ry; y++)
	{
		for (int x = cx - rx; x < cx + rx; x++)
		{
			float dx = (x + 0.5f - cx) / rx;
			float dy = (y + 0.5f - cy) / ry;
			if (dx * dx + dy * dy <= 1.0f)
				map.at(y, x) = TILE_FLOOR;
		}
	}

	// the largest centred rectangle that fits comfortably inside it
	int hx = std::max(1, static_cast<int>(rx * 0.7f));
	int hy = std::max(1, static_cast<int>(ry * 0.7f));
	Rect core{ cx - hx, cy - hy, hx * 2, hy * 2 };
	carve(map, core);
	return core;
}

} // namespace mapgen_detail

inline GeneratedMap generateMap(Uint64 seed, const MapGenSettings& s = MapGenSettings{})
{
	using namespace mapgen_detail;

	Rng rng{ seed };
	GeneratedMap map;
	map.seed = seed;
	map.cols = s.slotCols * s.slotSize;
	map.rows = s.slotRows * s.slotSize;
	map.cells.assign(map.rows * map.cols, TILE_EMPTY);

	const int slotCount = s.slotCols * s.slotRows;
	std::vector<bool> used(slotCount, false);
	std::vector<int> rooms;                  // slot indices, in the order they were added
	std::vector<std::pair<int, int>> links;  // pairs of slots to join with a corridor

	auto isLinked = [&](int a, int b) {
		for (auto& [p, q] : links)
			if ((p == a && q == b) || (p == b && q == a)) return true;
		return false;
	};

	// 1. grow the set of rooms outwards from a random slot
	int origin = rng.range(0, slotCount - 1);
	used[origin] = true;
	rooms.push_back(origin);

	const int dc[4] = { 1, -1, 0, 0 };
	const int dr[4] = { 0, 0, 1, -1 };
	int targetRooms = std::min(rng.range(s.minRooms, s.maxRooms), slotCount);

	for (int attempts = 0; static_cast<int>(rooms.size()) < targetRooms && attempts < 1000; attempts++)
	{
		// growing from the newest room stretches out long winding arms,
		// growing from any room fattens the layout into a blob -- mixing
		// the two is what keeps each level's silhouette different
		int from = rng.chance(0.5f) ? rooms.back() : rooms[rng.range(0, static_cast<int>(rooms.size()) - 1)];
		int d = rng.range(0, 3);
		int nc = from % s.slotCols + dc[d];
		int nr = from / s.slotCols + dr[d];
		if (nc < 0 || nc >= s.slotCols || nr < 0 || nr >= s.slotRows) continue;

		int to = nr * s.slotCols + nc;
		if (used[to]) continue;
		used[to] = true;
		rooms.push_back(to);
		links.push_back({ from, to });
	}

	// a growth tree on its own means every side branch is a dead end you
	// have to walk back out of, so add a few loops between rooms that
	// already sit next to each other
	for (int slot : rooms)
	{
		int right = slot + 1, down = slot + s.slotCols;
		if (slot % s.slotCols + 1 < s.slotCols && used[right] && !isLinked(slot, right) && rng.chance(s.loopChance))
			links.push_back({ slot, right });
		if (down < slotCount && used[down] && !isLinked(slot, down) && rng.chance(s.loopChance))
			links.push_back({ slot, down });
	}

	// 2. carve a room into each used slot
	std::vector<Rect> cores(slotCount);
	for (int slot : rooms)
	{
		int sx = slot % s.slotCols * s.slotSize;
		int sy = slot / s.slotCols * s.slotSize;

		// one tile of padding inside every edge of the slot, so rooms in
		// neighbouring slots always have at least two tiles of wall between
		// them unless a corridor deliberately joins them
		Rect area{ sx + 1, sy + 1, s.slotSize - 2, s.slotSize - 2 };
		int cx = sx + s.slotSize / 2;
		int cy = sy + s.slotSize / 2;

		cores[slot] = rng.chance(s.ellipseChance)
			? carveEllipseRoom(map, rng, area, cx, cy)
			: carveBlockRoom(map, rng, area, cx, cy);
	}

	// 3. straight two-wide corridors between linked cores, at a random
	// offset within the range both cores share so they don't all line up
	for (auto& [p, q] : links)
	{
		int a = std::min(p, q), b = std::max(p, q); // a is left of / above b
		const Rect& ca = cores[a];
		const Rect& cb = cores[b];

		if (a / s.slotCols == b / s.slotCols) // same lattice row: dig sideways
		{
			int top = rng.range(std::max(ca.y, cb.y), std::min(ca.y + ca.h, cb.y + cb.h) - 2);
			int x0 = ca.x + ca.w - 1;
			carve(map, Rect{ x0, top, cb.x - x0 + 1, 2 });
		}
		else // same lattice column: dig downwards
		{
			int left = rng.range(std::max(ca.x, cb.x), std::min(ca.x + ca.w, cb.x + cb.w) - 2);
			int y0 = ca.y + ca.h - 1;
			carve(map, Rect{ left, y0, 2, cb.y - y0 + 1 });
		}
	}

	// every floor tile has to belong to at least one 2x2 block of floor --
	// that's what keeps every passage at least two tiles wide. Rectangles
	// and corridors already guarantee it, this just trims any stray tip an
	// ellipse leaves behind.
	auto isFloor = [&](int r, int c) { return map.inBounds(r, c) && map.at(r, c) == TILE_FLOOR; };
	auto inTwoByTwo = [&](int r, int c) {
		for (int r0 = r - 1; r0 <= r; r0++)
			for (int c0 = c - 1; c0 <= c; c0++)
				if (isFloor(r0, c0) && isFloor(r0 + 1, c0) && isFloor(r0, c0 + 1) && isFloor(r0 + 1, c0 + 1))
					return true;
		return false;
	};
	for (bool changed = true; changed; )
	{
		changed = false;
		for (int r = 0; r < map.rows; r++)
			for (int c = 0; c < map.cols; c++)
				if (isFloor(r, c) && !inTwoByTwo(r, c))
				{
					map.at(r, c) = TILE_EMPTY;
					changed = true;
				}
	}

	// 4. wrap the floor in walls
	for (int r = 0; r < map.rows; r++)
	{
		for (int c = 0; c < map.cols; c++)
		{
			if (map.at(r, c) != TILE_EMPTY) continue;
			for (int nr = r - 1; nr <= r + 1; nr++)
				for (int nc = c - 1; nc <= c + 1; nc++)
					if (isFloor(nr, nc)) map.at(r, c) = TILE_WALL;
		}
	}

	// the player starts in the room furthest (in corridors walked) from
	// where growth began. That's usually a dead end at the tip of an arm,
	// so the level unfolds in front of them instead of all around them.
	// Breadth-first order means the last room reached is the furthest.
	std::vector<bool> reached(slotCount, false);
	std::vector<int> queue{ origin };
	reached[origin] = true;
	for (size_t i = 0; i < queue.size(); i++)
	{
		for (auto& [p, q] : links)
		{
			int next = p == queue[i] ? q : q == queue[i] ? p : -1;
			if (next != -1 && !reached[next])
			{
				reached[next] = true;
				queue.push_back(next);
			}
		}
	}
	int start = queue.back();

	const Rect& startCore = cores[start];
	map.at(startCore.y + startCore.h / 2, startCore.x + startCore.w / 2) = TILE_PLAYER;

	// enemies go on core tiles, which are always open floor
	for (int slot : rooms)
	{
		if (slot == start) continue;
		const Rect& core = cores[slot];
		int enemies = rng.range(0, s.maxEnemiesPerRoom);
		for (int i = 0; i < enemies; i++)
		{
			map.at(rng.range(core.y, core.y + core.h - 1), rng.range(core.x, core.x + core.w - 1)) = TILE_ENEMY;
		}
	}

	return map;
}
