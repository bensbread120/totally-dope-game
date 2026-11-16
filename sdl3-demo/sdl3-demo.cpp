// main.cpp : top-down adventure game prototype. Paul (player) and skeleton
// (enemy) each have idle / walk / attack clips in four directions.
//
#include "sdl3-demo.h"

using engine::SDLState;

const int TILE_SIZE = 32;

// skeletons stop to swing once they're about a tile from the player, telegraph
// for a moment so the player can react, then can't swing again until the cooldown ends
const float ENEMY_ATTACK_RANGE = 30.0f;
const float ENEMY_ATTACK_WINDUP = 0.35f;
const float ENEMY_ATTACK_COOLDOWN = 1.0f;

// Animation slots are laid out [clip][direction]: ANUM_<clip> + dirIndex(direction).
inline int dirIndex(Direction dir)
{
	switch (dir)
	{
		case Direction::Up:    return 1;
		case Direction::Left:  return 2;
		case Direction::Right: return 3;
		default:               return 0; // Down
	}
}

struct Resources
{
	static constexpr int ANUM_IDLE = 0;
	static constexpr int ANUM_WALK = 4;
	static constexpr int ANUM_ATTACK = 8;
	static constexpr int ANUM_COUNT = 12;

	// each animation has its own sprite strip; textures[i] goes with animations[i]
	std::vector<Animation> playerAnimations;
	std::vector<SDL_Texture*> playerTextures;
	std::vector<Animation> enemyAnimations;
	std::vector<SDL_Texture*> enemyTextures;

	engine::TextureCache cache;
	SDL_Texture* texWall, *texFloor;

	void load(SDLState& state)
	{
		auto tex = [&](const std::string& file) { return cache.load(state.renderer, "assets/" + file); };
		const char* dirs[] = { "down", "up", "left", "right" };

		// Strips are 32x32 frames, except attacks which are 48x48 so the swing
		// fits; drawObject centres oversized frames on the character's tile.
		playerAnimations.resize(ANUM_COUNT);
		playerTextures.resize(ANUM_COUNT);
		for (int d = 0; d < 4; d++)
		{
			playerAnimations[ANUM_IDLE + d] = Animation(4, 1.0f);
			playerAnimations[ANUM_WALK + d] = Animation(4, 0.5f);
			playerAnimations[ANUM_ATTACK + d] = Animation(4, 0.32f, /*loop*/false);
			// idle down is the hand-drawn original, hence the odd name out
			playerTextures[ANUM_IDLE + d] = tex(d == 0 ? std::string("paul_down.png") : std::format("paul_idle_{}.png", dirs[d]));
			playerTextures[ANUM_WALK + d] = tex(std::format("paul_walk_{}.png", dirs[d]));
			playerTextures[ANUM_ATTACK + d] = tex(std::format("paul_attack_{}.png", dirs[d]));
		}

		// The skeleton sheet has walk strips for up/down and run strips for
		// left/right, but idles only for left/right -- so idling up/down holds
		// the first walk frame.
		enemyAnimations.resize(ANUM_COUNT);
		enemyTextures.resize(ANUM_COUNT);
		for (int d = 0; d < 4; d++)
		{
			bool vertical = d < 2;
			enemyAnimations[ANUM_IDLE + d] = vertical ? Animation(1, 1.0f) : Animation(5, 0.5f);
			enemyAnimations[ANUM_WALK + d] = Animation(5, 0.5f);
			enemyAnimations[ANUM_ATTACK + d] = Animation(4, 0.45f, /*loop*/false);
			enemyTextures[ANUM_IDLE + d] = tex(std::format("skeleton_{}_{}.png", vertical ? "walk" : "idle", dirs[d]));
			enemyTextures[ANUM_WALK + d] = tex(std::format("skeleton_{}_{}.png", vertical ? "walk" : "run", dirs[d]));
			enemyTextures[ANUM_ATTACK + d] = tex(std::format("skeleton_attack_{}.png", dirs[d]));
		}

		texWall    = tex("panel.png");
		texFloor   = tex("brick.png");
	}

	void unload() { cache.unload(); }
};

// Switch to the clip for `anim` facing `dir`. `restart` rewinds it, which a
// one-shot clip like an attack needs every time it's played.
void playAnimation(engine::Entity& e, const std::vector<SDL_Texture*>& textures, int anim, Direction dir, bool restart = false)
{
	int idx = anim + dirIndex(dir);
	if (restart)
	{
		e.animations[idx].reset();
	}
	e.currentAnimation = idx;
	e.texture = textures[idx];
}

void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width, float height);
void startPlayerAttack(GameObject& obj, Resources& res);
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
				case SDL_EVENT_KEY_DOWN:
					// attacks are discrete actions, so they fire on the press
					// (not every frame the key is held, and not on key repeat)
					if (event.key.scancode == SDL_SCANCODE_SPACE && !event.key.repeat) {
						startPlayerAttack(gs.getPlayer(), resources);
					}
					break;
				case SDL_EVENT_KEY_UP:
					if (event.key.scancode == SDL_SCANCODE_1) {
						gs.debugMode = !gs.debugMode;
					}
					else if (event.key.scancode == SDL_SCANCODE_2) {
						initialiseGameState(state, gs, resources);
					}
					break;
			}
		}

		// movement is continuous (state.keys polled every frame in
		// updatePlayer); only the attack is edge-triggered, in the event loop above
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

		// back to front, so whoever is lower on screen overlaps the ones above
		// (attack frames are bigger than a tile and spill onto neighbours)
		std::vector<GameObject*> drawOrder;
		drawOrder.reserve(gs.characters.size());
		for (GameObject& character : gs.characters)
		{
			drawOrder.push_back(&character);
		}
		std::sort(drawOrder.begin(), drawOrder.end(), [](const GameObject* a, const GameObject* b) {
			return a->entity.position.y < b->entity.position.y;
		});
		for (GameObject* character : drawOrder)
		{
			drawObject(state, gs, *character, TILE_SIZE, TILE_SIZE);
		}

		if (gs.debugMode)
		{
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			SDL_RenderDebugText(state.renderer, 5, 5, std::format("walls: {}, characters: {}, seed: {}",
				gs.walls.size(), gs.characters.size(), gs.mapSeed).c_str());
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

	// width/height is the object's footprint (one tile). An animated object's
	// frame size comes from its strip instead: frames are square, so the strip's
	// height is the frame size. That way a strip with bigger frames (the 48x48
	// attacks) is drawn centred on the footprint, the body stays exactly where
	// the 32x32 clips put it, and an animation may use only the first few
	// frames of a strip (skeleton idle up/down holds walk frame 0).
	float frameW = width, frameH = height, srcX = 0.0f;
	if (e.currentAnimation != -1 && e.texture) // texture is null if its file failed to load
	{
		frameW = frameH = static_cast<float>(e.texture->h);
		srcX = e.animations[e.currentAnimation].currentFrame() * frameW;
	}
	SDL_FRect src{ srcX, 0, frameW, frameH };

	SDL_FRect dst{
		e.position.x - gs.camera.viewport.x - (frameW - width) / 2,
		e.position.y - gs.camera.viewport.y - (frameH - height) / 2,
		frameW, frameH
	};

	// every direction has its own strip, so no flipping
	SDL_RenderTexture(state.renderer, e.texture, &src, &dst);

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

	// planted for the whole swing: no moving or turning until it finishes
	if (player.attacking)
	{
		e.velocity = glm::vec2(0.0f);
		if (!e.animations[e.currentAnimation].isDone())
		{
			return;
		}
		player.attacking = false;
	}

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

	int clip = player.state == MovementState::Walking ? Resources::ANUM_WALK : Resources::ANUM_IDLE;
	playAnimation(e, res.playerTextures, clip, player.direction);
}

void startPlayerAttack(GameObject& obj, Resources& res)
{
	PlayerData& player = obj.data.player;
	if (player.attacking)
	{
		return; // no queueing or cancelling: one swing at a time
	}
	player.attacking = true;
	obj.entity.velocity = glm::vec2(0.0f);
	playAnimation(obj.entity, res.playerTextures, Resources::ANUM_ATTACK, player.direction, /*restart*/true);
}

// Direction whose axis dominates `v`, for turning towards something.
Direction dominantDirection(glm::vec2 v)
{
	return std::abs(v.x) > std::abs(v.y)
		? (v.x > 0.0f ? Direction::Right : Direction::Left)
		: (v.y > 0.0f ? Direction::Down : Direction::Up);
}

void updateEnemy(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime)
{
	engine::Entity& e = obj.entity;
	EnemyData& enemy = obj.data.enemy;

	// both sprites are the same size, so the gap between positions is the gap between centres
	glm::vec2 toPlayer = gs.getPlayer().entity.position - e.position;
	enemy.cooldownLeft = std::max(0.0f, enemy.cooldownLeft - deltaTime);

	switch (enemy.action)
	{
		case EnemyAction::Chase:
			if (enemy.cooldownLeft > 0.0f || glm::length(toPlayer) > ENEMY_ATTACK_RANGE)
			{
				break; // keep chasing (below)
			}
			// close enough: stop and telegraph the swing
			enemy.action = EnemyAction::Windup;
			enemy.windupLeft = ENEMY_ATTACK_WINDUP;
			[[fallthrough]];

		case EnemyAction::Windup:
			// stand still, tracking the player so the swing goes where they are now
			e.velocity = glm::vec2(0.0f);
			enemy.state = MovementState::Idle;
			if (toPlayer != glm::vec2(0.0f))
			{
				enemy.direction = dominantDirection(toPlayer);
			}
			e.facing = directionVector(enemy.direction);
			enemy.windupLeft -= deltaTime;
			if (enemy.windupLeft > 0.0f)
			{
				playAnimation(e, res.enemyTextures, Resources::ANUM_IDLE, enemy.direction);
				return;
			}
			// direction is locked from here until the swing ends
			enemy.action = EnemyAction::Attacking;
			playAnimation(e, res.enemyTextures, Resources::ANUM_ATTACK, enemy.direction, /*restart*/true);
			return;

		case EnemyAction::Attacking:
			e.velocity = glm::vec2(0.0f);
			if (!e.animations[e.currentAnimation].isDone())
			{
				return;
			}
			enemy.action = EnemyAction::Chase;
			enemy.cooldownLeft = ENEMY_ATTACK_COOLDOWN;
			break;
	}

	// for now will only chase player, but based on interactions can later update path target here
	gs.pathfinder.setTarget(gs.getPlayer().entity.position, e.id);
	gs.pathfinder.updatePath(e.id);

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
		enemy.direction = dominantDirection(toFace);
	}
	e.facing = directionVector(enemy.direction);

	int clip = enemy.state == MovementState::Walking ? Resources::ANUM_WALK : Resources::ANUM_IDLE;
	playAnimation(e, res.enemyTextures, clip, enemy.direction);
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
	// also used to regenerate mid-game, so throw away the previous level first
	gs.pathfinder = Pathfinder{};
	gs.walls.clear();
	gs.floorTiles.clear();
	gs.characters.clear();
	gs.playerIdx = -1;

	// a new layout every run. The seed is shown in debug mode -- pass it
	// here instead to replay a level you liked (or hit a bug in)
	GeneratedMap map = generateMap(SDL_GetPerformanceCounter());
	gs.mapSeed = map.seed;

	for (int r = 0; r < map.rows; r++)
	{
		for (int c = 0; c < map.cols; c++)
		{
			short cell = map.at(r, c);
			if (cell == TILE_EMPTY) continue; // outside the level, nothing to draw or collide with

			glm::vec2 pos(static_cast<float>(c * TILE_SIZE), static_cast<float>(r * TILE_SIZE));

			// every cell gets a floor sprite underneath it, walls included,
			// so there's no gap visible around or behind a wall tile
			gs.floorTiles.push_back(TileSprite{ pos, res.texFloor });

			switch (cell)
			{
				case TILE_WALL:
				{
					GameObject wall;
					wall.type = ObjectType::Wall;
					wall.entity.texture = res.texWall;
					wall.entity.position = pos;
					wall.entity.collider = SDL_FRect{ 0, 0, TILE_SIZE, TILE_SIZE };
					gs.walls.push_back(wall);
					break;
				}
				case TILE_PLAYER:
				{
					GameObject player;
					player.type = ObjectType::Player;
					player.data.player = PlayerData();
					player.entity.position = pos;
					player.entity.animations = res.playerAnimations;
					playAnimation(player.entity, res.playerTextures, Resources::ANUM_IDLE, Direction::Down);
					player.entity.maxSpeed = glm::vec2(120.0f, 120.0f);
					player.entity.collider = SDL_FRect{ 8, 8, 16, 16 };
					gs.characters.push_back(player);
					gs.playerIdx = static_cast<int>(gs.characters.size()) - 1;
					break;
				}
				case TILE_ENEMY:
				{
					GameObject enemy;
					enemy.type = ObjectType::Enemy;
					enemy.data.enemy = EnemyData();
					enemy.entity.position = pos;
					enemy.entity.animations = res.enemyAnimations;
					playAnimation(enemy.entity, res.enemyTextures, Resources::ANUM_IDLE, Direction::Down);
					enemy.entity.maxSpeed = glm::vec2(60.0f, 60.0f);
					enemy.entity.collider = SDL_FRect{ 8, 8, 16, 16 };
					gs.characters.push_back(enemy);
					gs.pathfinder.initialisePath(enemy);
					break;
				}
				default: break; // plain floor, nothing else to place
			}
		}
	}

	buildWallGrid(gs);
	gs.pathfinder.setBlockedCells(gs.walls);

	assert(gs.playerIdx != -1 && "Player not created in map");
}
