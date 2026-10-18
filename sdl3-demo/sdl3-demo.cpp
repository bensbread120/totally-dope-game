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

		updatePaths(gs);

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

		for (const TileSprite& tile : gs.floorTiles)
		{
			SDL_FRect dst{
				tile.position.x - gs.camera.viewport.x,
				tile.position.y - gs.camera.viewport.y,
				static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)
			};
			SDL_RenderTexture(state.renderer, tile.texture, nullptr, &dst);
		}

		for (GameObject& wall : gs.walls)
		{
			drawObject(state, gs, wall, TILE_SIZE, TILE_SIZE);
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
	
	enemy.state = MovementState::Walking;
	Path* path = findPath(gs.pathfinder, e.id);
	if (path == nullptr || path->pathIndex >= static_cast<int>(path->path.size())) { // no path, or reached its end
		enemy.state = MovementState::Idle;
		return;
	}

	enemy.direction = path->path[path->pathIndex];
	path->pathIndex += 1;

	// a wall collision below will zero this out again via resolveAxisAligned,
	// so a blocked enemy just sits at the wall until the timer next fires --
	// good enough for a placeholder, worth revisiting once you want an enemy
	// that turns around immediately instead of waiting
	glm::vec2 dir = enemy.state == MovementState::Walking ? directionVector(enemy.direction) : glm::vec2(0.0f);
	e.velocity = dir * e.maxSpeed;
	e.facing = directionVector(enemy.direction);
	e.texture = res.texEnemy;
	e.currentAnimation = res.ANUM_ENEMY_IDLE; // only one clip until it has its own walk animation
}

void moveAndCollide(GameState& gs, GameObject& obj, float deltaTime)
{
	obj.entity.position += obj.entity.velocity * deltaTime;

	// characters don't yet collide with each other, only with walls --
	// add a characters-vs-characters pass once you want combat/blocking
	for (GameObject& wall : gs.walls)
	{
		SDL_FRect overlap;
		if (engine::checkOverlap(obj.entity, wall.entity, overlap))
		{
			engine::resolveAxisAligned(obj.entity, overlap);
		}
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
					gs.pathfinder.targetLocation = player.entity.position;
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
					initialisePath(gs.pathfinder, enemy);
					break;
				}
				default: break; // 0 = floor, nothing else to place
			}
		}
	}

	assert(gs.playerIdx != -1 && "Player not created in map");
}
