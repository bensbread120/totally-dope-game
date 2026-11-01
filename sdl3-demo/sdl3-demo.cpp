// main.cpp : top-down adventure game prototype. Reuses the platformer's
// idle/run/enemy sprites as placeholders -- swap in real directional art
// and this file's rendering/animation lookups are the only things that
// need to change.
//
#include "sdl3-demo.h"

using engine::SDLState;

const int MAP_ROWS = 14;
const int MAP_COLS = 18;
const int TILE_SIZE = 32;

struct Resources
{
	const int ANUM_PLAYER_IDLE = 0;
	const int ANUM_PLAYER_IDLE_DOWN = 1;
	const int ANUM_PLAYER_WALK = 2;

	std::vector<Animation> playerAnimations;

	const int ANUM_ENEMY_IDLE = 0;
	std::vector<Animation> enemyAnimations;

	engine::TextureCache cache;
	SDL_Texture* texIdle, *texPaulDown, *texRunning, *texEnemy, *texWall, *texFloor;

	void load(SDLState& state)
	{
		playerAnimations.resize(3);
		playerAnimations[ANUM_PLAYER_IDLE] = Animation(8, 1.6f);
		playerAnimations[ANUM_PLAYER_IDLE_DOWN] = Animation(4, 1.0f);
		playerAnimations[ANUM_PLAYER_WALK] = Animation(4, 0.5f);

		enemyAnimations.resize(1);
		enemyAnimations[ANUM_ENEMY_IDLE] = Animation(8, 1.0f);


		// same source images as the platformer -- only the wall/floor
		// textures are new, and even those are borrowed from its tileset
		texIdle    = cache.load(state.renderer, "assets/idle.png");
		texPaulDown = cache.load(state.renderer, "assets/paul_down.png");
		texRunning = cache.load(state.renderer, "assets/run.png");
		texEnemy   = cache.load(state.renderer, "assets/enemy.png");
		texWall    = cache.load(state.renderer, "assets/panel.png");
		texFloor   = cache.load(state.renderer, "assets/brick.png");
	}

	void unload() { cache.unload(); }
};

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void updatePlayer(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void updateEnemy(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void moveAndCollide(GameState& gs, GameObject& obj, float deltaTime);
void buildWallGrid(GameState& gs);
void initialiseGameState(const SDLState& state, GameState& gs, Resources& res);

int main(int argc, char* argv[])
{
	SDLState state;
	state.width = 1152;
	state.height = 864;
	state.logW = 384;
	state.logH = 288;

	if (!engine::initialise(state, "SDL3 top-down demo")) {
		return 1;
	}

	Resources resources;
	resources.load(state);

	GameState gs;
	gs.camera.viewport = SDL_FRect{ 0, 0, static_cast<float>(state.logW), static_cast<float>(state.logH) };
	initialiseGameState(state, gs, resources);

	uint64_t previousTime = SDL_GetTicks();
	bool running = true;

	while (running)
	{
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - previousTime) / 1000.0f;

		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					running = false;
					break;
				case SDL_EVENT_WINDOW_RESIZED:
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
				case SDL_EVENT_KEY_UP:
					if (event.key.scancode == SDL_SCANCODE_1) {
						gs.debugMode = !gs.debugMode;
					}
					break;
			}
		}

		// movement here is continuous (state.keys polled every frame in
		// updatePlayer) rather than the platformer's edge-triggered jump,
		// so there's no handleKeyInput -- that pattern comes back once you
		// add a discrete action like an attack.
		for (GameObject& character : gs.characters)
		{
			update(state, gs, resources, character, deltaTime);
		}

		gs.camera.centerOn(gs.getPlayer().entity.position, TILE_SIZE, TILE_SIZE, /*followX*/true, /*followY*/true);

		SDL_SetRenderDrawColor(state.renderer, 20, 20, 25, 255);
		SDL_RenderClear(state.renderer);

		// only draw what the camera can see; big levels have thousands of tiles off-screen
		const SDL_FRect& view = gs.camera.viewport;
		auto onScreen = [&view](glm::vec2 pos) {
			return pos.x + TILE_SIZE > view.x && pos.x < view.x + view.w
				&& pos.y + TILE_SIZE > view.y && pos.y < view.y + view.h;
		};

		for (const TileSprite& tile : gs.floorTiles)
		{
			if (!onScreen(tile.position))
			{
				continue;
			}

			SDL_FRect dst{
				tile.position.x - gs.camera.viewport.x,
				tile.position.y - gs.camera.viewport.y,
				static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)
			};
			SDL_RenderTexture(state.renderer, tile.texture, nullptr, &dst);
		}

		for (GameObject& wall : gs.walls)
		{
			if (onScreen(wall.entity.position))
			{
				drawObject(state, gs, wall, TILE_SIZE, TILE_SIZE);
			}
		}

		for (GameObject& character : gs.characters)
		{
			drawObject(state, gs, character, TILE_SIZE, TILE_SIZE);
		}

		if (gs.debugMode)
		{
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			SDL_RenderDebugText(state.renderer, 5, 5, std::format("walls: {}, characters: {}",
				gs.walls.size(), gs.characters.size()).c_str());
		}

		SDL_RenderPresent(state.renderer);
		previousTime = nowTime;
	}

	resources.unload();
	engine::cleanup(state);
	return 0;
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height)
{
	engine::Entity& e = obj.entity;
	float srcX = e.currentAnimation != -1 ? e.animations[e.currentAnimation].currentFrame() * width : 0.0f;
	SDL_FRect src{ srcX, 0, width, height };

	SDL_FRect dst{
		e.position.x - gs.camera.viewport.x,
		e.position.y - gs.camera.viewport.y,
		width, height
	};

	// Only one facing sprite exists so far, so this is a placeholder: flip
	// horizontally for Left, leave Right/Up/Down as the default pose. Once
	// you have real per-direction art, key this off e.facing.x/y properly
	// (or just pick the right texture per direction, no flip needed).
	SDL_FlipMode flipMode = (e.facing.x < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, e.texture, &src, &dst, 0, nullptr, flipMode);

	if (gs.debugMode)
	{
		SDL_FRect worldCollider = e.worldCollider();
		SDL_FRect rect{
			worldCollider.x - gs.camera.viewport.x,
			worldCollider.y - gs.camera.viewport.y,
			worldCollider.w, worldCollider.h
		};
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 100);
		SDL_RenderFillRect(state.renderer, &rect);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	switch (obj.type)
	{
		case ObjectType::Player: updatePlayer(state, gs, res, obj, deltaTime); break;
		case ObjectType::Enemy:  updateEnemy(state, gs, res, obj, deltaTime); break;
		default: return; // walls don't update
	}
	moveAndCollide(gs, obj, deltaTime);
	obj.entity.stepAnimation(deltaTime);
}

void updatePlayer(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	engine::Entity& e = obj.entity;
	PlayerData& player = obj.data.player;

	glm::vec2 input{ 0.0f };
	if (state.keys[SDL_SCANCODE_LEFT])  input.x -= 1.0f;
	if (state.keys[SDL_SCANCODE_RIGHT]) input.x += 1.0f;
	if (state.keys[SDL_SCANCODE_UP])    input.y -= 1.0f;
	if (state.keys[SDL_SCANCODE_DOWN])  input.y += 1.0f;

	if (input.x != 0.0f || input.y != 0.0f)
	{
		input = glm::normalize(input); // so diagonals aren't faster than cardinals
		player.state = MovementState::Walking;

		// dominant axis picks one of four sprite directions, even when
		// the input itself is diagonal
		if (std::abs(input.x) >= std::abs(input.y))
			player.direction = input.x < 0 ? Direction::Left : Direction::Right;
		else
			player.direction = input.y < 0 ? Direction::Up : Direction::Down;
	}
	else
	{
		player.state = MovementState::Idle;
		// direction is left as whatever it last was -- standing still,
		// you keep facing the way you were walking
	}

	// arcade-style movement: no acceleration/drift, matches most top-down
	// action-adventure games better than the platformer's momentum
	e.velocity = input * e.maxSpeed;
	e.facing = directionVector(player.direction);

	if (player.state == MovementState::Walking)
	{

		e.texture = res.texRunning;
		e.currentAnimation = res.ANUM_PLAYER_WALK;
	}
	else
	{
		switch (player.direction)
		{
			case Direction::Down:
			{
				e.texture = res.texPaulDown;
				e.currentAnimation = res.ANUM_PLAYER_IDLE_DOWN;
				break;
			}
			case Direction::Up:
			case Direction::Left:
			case Direction::Right:
			{
				e.texture = res.texIdle;
				e.currentAnimation = res.ANUM_PLAYER_IDLE;
			}
		}
		
	}
}

void updateEnemy(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	engine::Entity& e = obj.entity;
	EnemyData& enemy = obj.data.enemy;
	
	// for now will only chase player, but based on interactions can later update path target here
	gs.pathfinder.setTarget(gs.getPlayer().entity.position, e.id);
	gs.pathfinder.updatePath(e.id);

	e.texture = res.texEnemy;
	e.currentAnimation = res.ANUM_ENEMY_IDLE; // only one clip until it has its own walk animation

	// follow the path cell by cell (or hold our cell while the way ahead is taken),
	// then once the grid can't get us any closer, close in on the exact target position
	glm::ivec2 moveCell;
	Pathfinder::Step step = gs.pathfinder.nextStep(e.id, gs.characters, moveCell);
	glm::vec2 targetPosition = gs.pathfinder.findPath(e.id)->targetPosition;
	glm::vec2 goal = step == Pathfinder::Step::Arrived ? targetPosition : glm::vec2(moveCell * AStar::TILE_SIZE);

	// land exactly on the goal this frame rather than overshooting it
	glm::vec2 toGoal = goal - e.position;
	float distance = glm::length(toGoal);
	float stepLength = e.maxSpeed.x * deltaTime;
	bool reachesGoal = distance <= stepLength;
	glm::vec2 move = reachesGoal ? toGoal : toGoal / distance * stepLength;

	if (step == Pathfinder::Step::Arrived) {
		// off the grid now, so stop flush against anyone in the way instead of overlapping them
		SDL_FRect body = e.worldCollider();
		float allowed = 1.0f;
		for (const GameObject& other : gs.characters) {
			if (other.entity.id != e.id) {
				allowed = std::min(allowed, engine::sweepContactTime(body, move, other.entity.worldCollider()));
			}
		}
		move *= allowed;
	}
	else if (step == Pathfinder::Step::Walk && reachesGoal) {
		// only move on to the following step once we land on the cell, so each path step covers a whole tile
		gs.pathfinder.advance(e.id);
	}
	e.velocity = deltaTime > 0.0f ? move / deltaTime : glm::vec2(0.0f);

	// face the way we're moving, or the target once we've stopped
	enemy.state = glm::length(move) > 0.001f ? MovementState::Walking : MovementState::Idle;
	glm::vec2 toFace = enemy.state == MovementState::Walking ? move : targetPosition - e.position;
	if (toFace != glm::vec2(0.0f)) {
		enemy.direction = std::abs(toFace.x) > std::abs(toFace.y)
			? (toFace.x > 0.0f ? Direction::Right : Direction::Left)
			: (toFace.y > 0.0f ? Direction::Down : Direction::Up);
	}
	e.facing = directionVector(enemy.direction);
}

void moveAndCollide(GameState& gs, GameObject& obj, float deltaTime)
{
	obj.entity.position += obj.entity.velocity * deltaTime;

	// characters don't yet collide with each other, only with walls --
	// add a characters-vs-characters pass once you want combat/blocking.
	// walls sit one per tile, so only the cells under our collider can hold one we touch
	SDL_FRect body = obj.entity.worldCollider();
	int firstCol = static_cast<int>(std::floor(body.x / TILE_SIZE)) - gs.wallGridOrigin.x;
	int lastCol = static_cast<int>(std::floor((body.x + body.w) / TILE_SIZE)) - gs.wallGridOrigin.x;
	int firstRow = static_cast<int>(std::floor(body.y / TILE_SIZE)) - gs.wallGridOrigin.y;
	int lastRow = static_cast<int>(std::floor((body.y + body.h) / TILE_SIZE)) - gs.wallGridOrigin.y;
	for (int r = std::max(firstRow, 0); r <= std::min(lastRow, gs.wallGridRows - 1); r++)
	{
		for (int c = std::max(firstCol, 0); c <= std::min(lastCol, gs.wallGridCols - 1); c++)
		{
			int wallIdx = gs.wallGrid[r * gs.wallGridCols + c];
			if (wallIdx == -1)
			{
				continue;
			}

			SDL_FRect overlap;
			if (engine::checkOverlap(obj.entity, gs.walls[wallIdx].entity, overlap))
			{
				engine::resolveAxisAligned(obj.entity, overlap);
			}
		}
	}
}

void buildWallGrid(GameState& gs)
{
	gs.wallGrid.clear();
	gs.wallGridCols = gs.wallGridRows = 0;
	if (gs.walls.empty())
	{
		return;
	}

	// size the grid to the walls' bounding box
	auto cellOf = [](const GameObject& wall) {
		return glm::ivec2(glm::floor(wall.entity.position / static_cast<float>(TILE_SIZE)));
	};
	glm::ivec2 minCell = cellOf(gs.walls[0]);
	glm::ivec2 maxCell = minCell;
	for (const GameObject& wall : gs.walls)
	{
		minCell = glm::min(minCell, cellOf(wall));
		maxCell = glm::max(maxCell, cellOf(wall));
	}
	gs.wallGridOrigin = minCell;
	gs.wallGridCols = maxCell.x - minCell.x + 1;
	gs.wallGridRows = maxCell.y - minCell.y + 1;

	gs.wallGrid.assign(gs.wallGridCols * gs.wallGridRows, -1);
	for (size_t i = 0; i < gs.walls.size(); i++)
	{
		glm::ivec2 cell = cellOf(gs.walls[i]) - minCell;
		gs.wallGrid[cell.y * gs.wallGridCols + cell.x] = static_cast<int>(i);
	}
}

void initialiseGameState(const SDLState& state, GameState& gs, Resources& res)
{
	gs.pathfinder = Pathfinder{};
	
	/*
		0 = floor (walkable)
		1 = wall (blocks movement)
		2 = player start
		3 = enemy start
	*/
	short map[MAP_ROWS][MAP_COLS] = {
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
		1,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,1,
		1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,0,0,0,3,0,0,0,0,2,0,0,0,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,1,1,0,0,0,0,3,0,0,0,1,1,0,0,1,
		1,0,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	};

	for (int r = 0; r < MAP_ROWS; r++)
	{
		for (int c = 0; c < MAP_COLS; c++)
		{
			glm::vec2 pos(static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE));

			// every cell gets a floor sprite underneath it, walls included,
			// so there's no gap visible around or behind a wall tile
			gs.floorTiles.push_back(TileSprite{ pos, res.texFloor });

			switch (map[r][c])
			{
				case 1: // wall
				{
					GameObject wall;
					wall.type = ObjectType::Wall;
					wall.entity.texture = res.texWall;
					wall.entity.position = pos;
					wall.entity.collider = SDL_FRect{ 0, 0, TILE_SIZE, TILE_SIZE };
					gs.walls.push_back(wall);
					break;
				}
				case 2: // player start
				{
					GameObject player;
					player.type = ObjectType::Player;
					player.data.player = PlayerData();
					player.entity.position = pos;
					player.entity.texture = res.texIdle;
					player.entity.animations = res.playerAnimations;
					player.entity.currentAnimation = res.ANUM_PLAYER_IDLE;
					player.entity.maxSpeed = glm::vec2(120.0f, 120.0f);
					player.entity.collider = SDL_FRect{ 8, 8, 16, 16 };
					gs.characters.push_back(player);
					gs.playerIdx = static_cast<int>(gs.characters.size()) - 1;
					break;
				}
				case 3: // enemy start
				{
					GameObject enemy;
					enemy.type = ObjectType::Enemy;
					enemy.data.enemy = EnemyData();
					enemy.entity.position = pos;
					enemy.entity.texture = res.texEnemy;
					enemy.entity.animations = res.enemyAnimations;
					enemy.entity.currentAnimation = res.ANUM_ENEMY_IDLE;
					enemy.entity.maxSpeed = glm::vec2(60.0f, 60.0f);
					enemy.entity.collider = SDL_FRect{ 8, 8, 16, 16 };
					gs.characters.push_back(enemy);
					gs.pathfinder.initialisePath(enemy);
					break;
				}
				default: break; // 0 = floor, nothing else to place
			}
		}
	}

	buildWallGrid(gs);
	gs.pathfinder.setBlockedCells(gs.walls);

	assert(gs.playerIdx != -1 && "Player not created in map");
}
