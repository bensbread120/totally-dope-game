// sdl3-demo.cpp : Defines the entry point for the application.
//
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "sdl3-demo.h"
#include "gameobject.h"
#include "engine/collision.h"
#include "engine/camera.h"
#include "engine/texture_cache.h"
#include <vector>
#include <string>
#include <array>
#include <format>

using namespace std;

/// <summary>
/// 
/// </summary>
struct SDLState
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
	const bool* keys;

	SDLState() : keys(SDL_GetKeyboardState(nullptr))
	{ }
};

const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

/// <summary>
/// 
/// </summary>
struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	std::vector<GameObject> backgroundTiles;
	std::vector<GameObject> foregroundTiles;
	std::vector<GameObject> bullets;
	int playerIdx;
	engine::Camera camera;
	float bg2Scroll, bg3Scroll, bg4Scroll;
	bool debugMode;

	GameState(const SDLState &state) {
		playerIdx = -1; // changed when loaded
		camera.viewport = SDL_FRect{
			.x = 0, .y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH)
		};
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
		debugMode = false;
	}

	GameObject& getPlayer() {
		return layers[LAYER_IDX_CHARACTERS][playerIdx];
	}
};

/// <summary>
/// 
/// </summary>
struct Resources
{
	const int ANUM_PLAYER_IDLE = 0;
	const int ANUM_PLAYER_RUNNING = 1;
	const int ANUM_PLAYER_SLIDE = 2;
	const int ANUM_PLAYER_SHOOT = 3;
	const int ANUM_PLAYER_SLIDE_SHOOT = 4;
	std::vector<Animation> animations;
	const int ANUM_BULLET_MOVING = 0;
	const int ANUM_BULLET_HIT = 1;
	std::vector<Animation> bulletAnimations;
	const int ANUM_ENEMY = 0;
	const int ANUM_ENEMY_HIT = 1;
	const int ANUM_ENEMY_DIE = 2;
	std::vector<Animation> enemyAnimations;

	engine::TextureCache cache;
	SDL_Texture* texIdle, *texRunning, *texBrick, *texGrass, *texGround, *texPanel,
		*texSlide, *texbg1, *texbg2, *texbg3, *texbg4, *texBullet, *texBulletHit,
		*texShoot, *texRunShoot, *texSlideShoot, *texEnemy, *texEnemyHit, *texEnemyDie;

	void load(SDLState& state)
	{
		SDL_Log("Base path: %s", SDL_GetBasePath());
		SDL_Log("Current directory: %s", SDL_GetCurrentDirectory());
		animations.resize(5);
		animations[ANUM_PLAYER_IDLE] = Animation(8, 1.6f); // 8 frames, 1.6 second long
		animations[ANUM_PLAYER_RUNNING] = Animation(4, 0.5f); // 4 frames, 0.5 second long
		animations[ANUM_PLAYER_SLIDE] = Animation(1, 1.0f);
		animations[ANUM_PLAYER_SHOOT] = Animation(4, 0.5f);
		animations[ANUM_PLAYER_SLIDE_SHOOT] = Animation(4, 0.5f);
		bulletAnimations.resize(2);
		bulletAnimations[ANUM_BULLET_MOVING] = Animation(4, 0.05f);
		bulletAnimations[ANUM_BULLET_HIT] = Animation(4, 0.15f);
		enemyAnimations.resize(3);
		enemyAnimations[ANUM_ENEMY] = Animation(8, 1.0f);
		enemyAnimations[ANUM_ENEMY_HIT] = Animation(8, 1.0f);
		enemyAnimations[ANUM_ENEMY_DIE] = Animation(18, 2.0f);

		texIdle = cache.load(state.renderer, "assets/idle.png");
		texRunning = cache.load(state.renderer, "assets/run.png");
		texBrick = cache.load(state.renderer, "assets/brick.png");
		texGrass = cache.load(state.renderer, "assets/grass.png");
		texGround = cache.load(state.renderer, "assets/ground.png");
		texPanel = cache.load(state.renderer, "assets/panel.png");
		texSlide = cache.load(state.renderer, "assets/slide.png");
		texbg1 = cache.load(state.renderer, "assets/bg_layer1.png");
		texbg2 = cache.load(state.renderer, "assets/bg_layer2.png");
		texbg3 = cache.load(state.renderer, "assets/bg_layer3.png");
		texbg4 = cache.load(state.renderer, "assets/bg_layer4.png");
		texBullet = cache.load(state.renderer, "assets/bullet.png");
		texBulletHit = cache.load(state.renderer, "assets/bullet_hit.png");
		texShoot = cache.load(state.renderer, "assets/shoot.png");
		texRunShoot = cache.load(state.renderer, "assets/shoot_run.png");
		texSlideShoot = cache.load(state.renderer, "assets/slide_shoot.png");
		texEnemy = cache.load(state.renderer, "assets/enemy.png");
		texEnemyHit = cache.load(state.renderer, "assets/enemy_hit.png");
		texEnemyDie = cache.load(state.renderer, "assets/enemy_die.png");
	}

	void unload() {
		cache.unload();
	}
};

bool initialise(SDLState& state);
void cleanup(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj,
	float width, float height, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createtiles(const SDLState& state, GameState& gs, Resources& res);
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& obj1, GameObject& obj2, float deltaTime);
void collisionResponse(const SDLState& state, GameState& gs, Resources& res,
	const SDL_FRect& overlap, GameObject& obj1, GameObject& obj2, float deltaTime);
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool isPressed);
void DrawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture,
	float xVelocity, float& scrollPos, float scrollFactor, float deltaTime);
/// <summary>
/// 
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, char* argv[])
{
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;

	if (!initialise(state)) {
		return 1;
	}

	// load game assets
	Resources resources;
	resources.load(state);
	// setup game state
	GameState gs(state);
	createtiles(state, gs, resources);

	uint64_t previousTime = SDL_GetTicks();

	// Game loop begin
	bool running = true;
	while (running) {
		
		uint64_t nowTime = SDL_GetTicks();
		float deltaTime = (nowTime - previousTime) / 1000.0f; // convert to seconds

		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_EVENT_QUIT: {
					running = false;
					break;
				}
				case SDL_EVENT_WINDOW_RESIZED: {
					state.width = event.window.data1;
					state.height = event.window.data2;
					break;
				}
				case SDL_EVENT_KEY_DOWN: {
					if (event.key.repeat == 0) {
						handleKeyInput(state, gs, gs.getPlayer(), event.key.scancode, true);
					}
					break;
				}
				case SDL_EVENT_KEY_UP: {
					handleKeyInput(state, gs, gs.getPlayer(), event.key.scancode, false);
					if (event.key.scancode == SDL_SCANCODE_1) {
						gs.debugMode = !gs.debugMode;
					}
					break;
				}
			}
		}

		// update all game objects
		for (auto& layer : gs.layers) {
			for (GameObject& obj : layer) {
				update(state, gs, resources, obj, deltaTime);
				// update animations
				obj.entity.stepAnimation(deltaTime);
			}
		}

		// update bullets
		for (GameObject& bullet : gs.bullets) {
			update(state, gs, resources, bullet, deltaTime);
			// update animations
			bullet.entity.stepAnimation(deltaTime);
		}

		// calculate viewport position
		gs.camera.centerOn(gs.getPlayer().entity.position, TILE_SIZE, TILE_SIZE, /*followX*/true, /*followY*/false);

		// perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 55, 55, 255);
		SDL_RenderClear(state.renderer);

		// draw background images
		SDL_RenderTexture(state.renderer, resources.texbg1, nullptr, nullptr);
		DrawParalaxBackground(state.renderer, resources.texbg4, gs.getPlayer().entity.velocity.x, gs.bg4Scroll, 0.075f, deltaTime);
		DrawParalaxBackground(state.renderer, resources.texbg3, gs.getPlayer().entity.velocity.x, gs.bg3Scroll, 0.15f, deltaTime);
		DrawParalaxBackground(state.renderer, resources.texbg2, gs.getPlayer().entity.velocity.x, gs.bg2Scroll, 0.3f, deltaTime);
		
		for (GameObject& obj : gs.backgroundTiles)
		{
			SDL_FRect dst{
				.x = obj.entity.position.x - gs.camera.viewport.x, .y = obj.entity.position.y,
				.w = static_cast<float>(obj.entity.texture->w),
				.h = static_cast<float>(obj.entity.texture->h),
			};
			SDL_RenderTexture(state.renderer, obj.entity.texture, nullptr, &dst);
		}

		// draw all objects
		for (auto& layer : gs.layers) {
			for (GameObject &obj : layer) {
				drawObject(state, gs, obj, TILE_SIZE, TILE_SIZE, deltaTime);
			}
		}

		// draw bullets
		for (GameObject& bullet : gs.bullets) {
			if (bullet.data.bullet.state != BulletState::inactive) {
				drawObject(state, gs, bullet, bullet.entity.collider.w, bullet.entity.collider.w, deltaTime);
			}
		}

		for (GameObject& obj : gs.foregroundTiles)
		{
			SDL_FRect dst{
				.x = obj.entity.position.x - gs.camera.viewport.x, .y = obj.entity.position.y,
				.w = static_cast<float>(obj.entity.texture->w),
				.h = static_cast<float>(obj.entity.texture->h),
			};
			SDL_RenderTexture(state.renderer, obj.entity.texture, nullptr, &dst);
		}

		// debug section
		if (gs.debugMode) {
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			SDL_RenderDebugText(state.renderer, 5, 5, std::format("S: {}, B: {}, G: {}",
				static_cast<int>(gs.getPlayer().data.player.state), gs.bullets.size(), gs.getPlayer().grounded).c_str());
  
		}

		// swap buffer and present
		SDL_RenderPresent(state.renderer);
		previousTime = nowTime;
	}

	resources.unload();
	cleanup(state);
	return 0;
}

////////////////////////////////////////////
// Methods
////////////////////////////

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
/// <returns></returns>
bool initialise(SDLState& state) {
	bool initSuccess = true;

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL", nullptr);
		initSuccess = false;
	}

	// Create window
	state.window = SDL_CreateWindow("SDL3 demo", state.width, state.height, SDL_WINDOW_RESIZABLE);

	if (!state.window)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating window", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	// create window renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error creating renderer", nullptr);
		cleanup(state);
		initSuccess = false;
	}

	SDL_SetRenderVSync(state.renderer, 1);

	// configure presentation
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	
	return initSuccess;
}

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
void cleanup(SDLState &state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
/// <param name="gs"></param>
/// <param name="obj"></param>
/// <param name="deltaTime"></param>
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, 
	float width, float height, float deltaTime) {

	engine::Entity& e = obj.entity;
	float srcX = e.currentAnimation != -1 ? e.animations[e.currentAnimation].currentFrame() * width : 0.0f;
	SDL_FRect src{
		.x = srcX,
		.y = 0,
		.w = width,
		.h = height
	};

	SDL_FRect dst{
		.x = e.position.x - gs.camera.viewport.x,
		.y = e.position.y,
		.w = width,
		.h = height
	};

	SDL_FlipMode flipMode = (e.facing.x < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, e.texture, &src, &dst, 0, nullptr, flipMode);

	// debug collider
	if (gs.debugMode) {
		SDL_FRect worldCollider = e.worldCollider();
		SDL_FRect RectA{
			.x = worldCollider.x - gs.camera.viewport.x,
			.y = worldCollider.y,
			.w = worldCollider.w,
			.h = worldCollider.h
		};
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 100);
		SDL_RenderFillRect(state.renderer, &RectA);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
/// <param name="gs"></param>
/// <param name="res"></param>
/// <param name="obj"></param>
/// <param name="deltaTime"></param>
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) 
{
	engine::Entity& e = obj.entity;

	if (e.dynamic && !obj.grounded) {
		// apply gravity -- purely a platformer thing; a top-down game has
		// no "down" to fall towards, so this stays out of engine::Entity
		// on purpose rather than being a generic "physics" behaviour.
		e.velocity += glm::vec2(0, 500) * deltaTime;
	}
	
	float currentDirection = 0;
	if (obj.type == ObjectType::Player) {
		// Update player-specific logic
		
		if (state.keys[SDL_SCANCODE_LEFT]) {
			currentDirection -= 1.0f;
		}
		if (state.keys[SDL_SCANCODE_RIGHT]) {
			currentDirection += 1.0f;
		}
		if (state.keys[SDL_SCANCODE_ESCAPE]) {
			// exit game
		}

		Timer& weaponTimer = obj.data.player.weaponTimer;
		weaponTimer.step(deltaTime);

		const auto handleShooting = [&state, &gs, &res, &obj, &e, &weaponTimer](
			SDL_Texture *tex, SDL_Texture *shootTex, int animIndex, int shootAnimIndex)
		{
			if (state.keys[SDL_SCANCODE_D])
			{
				// set shooting tex/anim
				e.texture = shootTex;
				e.currentAnimation = shootAnimIndex;

				if (weaponTimer.isTimeout())
				{
					weaponTimer.reset();
					// fire bullet
					GameObject bullet;
					bullet.data.bullet = BulletData();
					bullet.type = ObjectType::Bullet;
					bullet.entity.facing.x = e.facing.x;
					bullet.entity.texture = res.texBullet;
					bullet.entity.currentAnimation = res.ANUM_BULLET_MOVING;
					bullet.entity.collider = SDL_FRect{
						.x = 0, .y = 0,
						.w = static_cast<float>(res.texBullet->h),
						.h = static_cast<float>(res.texBullet->h)
					};
					const int yVariation = 40;
					const float yVelocity = SDL_rand(yVariation) - yVariation / 2.0f;

					bullet.entity.velocity = glm::vec2(
						e.velocity.x + 600.0f * e.facing.x,
						yVelocity
					);
					bullet.entity.maxSpeed.x = 1000.0f;
					bullet.entity.animations = res.bulletAnimations;

					// adjust bullet start pos
					const float left = 4;
					const float right = 24;
					const float t = (e.facing.x + 1) / 2.0f; // result in a value of 0..1
					const float xOffset = left + right * t;
					bullet.entity.position = glm::vec2(
						e.position.x + xOffset,
						e.position.y + TILE_SIZE / 2 + 1

					);
					bool foundInactive = false;
					for (int i = 0; i < gs.bullets.size() && !foundInactive; i++) {
						if (gs.bullets[i].data.bullet.state == BulletState::inactive) {
							foundInactive = true;
							gs.bullets[i] = bullet;
						}
					}

					// if not inactive bullet found.
					if (!foundInactive) {
						gs.bullets.push_back(bullet);
					}
				}
			}
			else {
				e.texture = tex;
				e.currentAnimation = animIndex;
			}
		};
		
		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (currentDirection)
				{
					obj.data.player.state = PlayerState::running;
				}
				else {
					// deccelerate to stop
					if (e.velocity.x)
					{
						const float decceeration = e.velocity.x > 0 ? -1.5f : 1.5f;
						float amount = decceeration * e.acceleration.x * deltaTime;
						if (std::abs(amount) > std::abs(e.velocity.x)) 
						{
							e.velocity.x = 0;
						}
						else {
							e.velocity.x += amount;
						}
					}
				}
				handleShooting(res.texIdle, res.texShoot, res.ANUM_PLAYER_IDLE, res.ANUM_PLAYER_SHOOT);
				break;
			}
			case PlayerState::running:
			{
				// switch to idle
				if (!currentDirection)
				{
					obj.data.player.state = PlayerState::idle;
				}
				// moving in opposite direction of velocity, sliding
				if (e.velocity.x * e.facing.x < 0 && obj.grounded) 
				{
					handleShooting(res.texSlide, res.texSlideShoot, res.ANUM_PLAYER_SLIDE, res.ANUM_PLAYER_SLIDE_SHOOT);
				}
				else 
				{
					handleShooting(res.texRunning, res.texRunShoot, res.ANUM_PLAYER_RUNNING, res.ANUM_PLAYER_RUNNING);
				}
				break;
			}
			case PlayerState::jumping:
			{
				handleShooting(res.texRunning, res.texRunShoot, res.ANUM_PLAYER_RUNNING, res.ANUM_PLAYER_RUNNING);
				break;
			}
		}	
	}
	else if (obj.type == ObjectType::Bullet) {
		switch (obj.data.bullet.state)
		{
			case BulletState::moving:
			{
				if (e.position.x - gs.camera.viewport.x < 0 || // left edge
					e.position.x - gs.camera.viewport.x > state.logW || // right edge
					e.position.y - gs.camera.viewport.y < 0 || // top edge
					e.position.y - gs.camera.viewport.y > state.logH) // bottom edge
				{ 
					obj.data.bullet.state = BulletState::inactive;
				}
				break;
			}
			case BulletState::colliding:
			{
				if (e.animations[e.currentAnimation].isDone())
				{
					obj.data.bullet.state = BulletState::inactive;
				}
				break;
			}
		}
		
	}
	if (currentDirection)
	{
		e.facing.x = currentDirection;
	}
	// add acceleration to velocity -- left as platformer-specific: this
	// clamp snaps velocity.x to (currentDirection * maxSpeed.x), which only
	// makes sense for 1D, input-driven horizontal movement (notice it zeroes
	// X velocity whenever currentDirection is 0, even mid-slide). The
	// top-down game should get its own version of this once you've settled
	// on how its input works, rather than sharing this one.
	e.velocity += currentDirection * e.acceleration * deltaTime;
	if (std::abs(e.velocity.x) > e.maxSpeed.x) {
		e.velocity.x = currentDirection * e.maxSpeed.x;
	}

	// add velocity to position
	e.position += e.velocity * deltaTime;
	bool foundGround = false;
	for (auto& layer : gs.layers) {
		for (GameObject& other : layer) {
			if (&obj != &other) {
				checkCollision(state, gs, res, obj, other, deltaTime);

				if (other.type == ObjectType::level) {
					// check if player is grounded
					SDL_FRect sensor{
						.x = e.position.x + e.collider.x,
						.y = e.position.y + e.collider.y + e.collider.h,
						.w = e.collider.w,
						.h = 1.5f // 1 pixel height sensor
					};
					SDL_FRect rectB = other.entity.worldCollider();
					SDL_FRect rectC{ 0 };
					if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC)) {
						foundGround = true;
					}
				}
			}
		}
	}
	if (obj.grounded != foundGround)  {
		obj.grounded = foundGround;
		if (foundGround && obj.type == ObjectType::Player) {
			obj.data.player.state = PlayerState::running;
		}
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
/// <param name="gs"></param>
/// <param name="res"></param>
/// <param name="overlap"></param>
/// <param name="obj1"></param>
/// <param name="obj2"></param>
/// <param name="deltaTime"></param>
void collisionResponse(const SDLState& state, GameState& gs, Resources& res,
	const SDL_FRect& overlap, GameObject& obj1, GameObject& obj2, float deltaTime) 
{
	if (obj1.type == ObjectType::Player)
	{
		switch (obj2.type)
		{
			case (ObjectType::level):
			{
				// resolve collision with level object
				engine::resolveAxisAligned(obj1.entity, overlap);
				break;
			}
		}
	}
	else if (obj1.type == ObjectType::Bullet) {
		switch (obj1.data.bullet.state)
		{
			case BulletState::moving:
			{
				switch (obj2.type)
				{
					case ObjectType::level:
					{
						break;
					}
					case ObjectType::Enemy:
					{
						obj2.entity.facing.x = -obj1.entity.facing.x;
					}
				}
				engine::resolveAxisAligned(obj1.entity, overlap);
				obj1.entity.velocity *= 0;
				obj1.data.bullet.state = BulletState::colliding;
				obj1.entity.currentAnimation = res.ANUM_BULLET_HIT;
				obj1.entity.texture = res.texBulletHit;
				break;
			}
		}
	}
}

void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& obj1, GameObject& obj2, float deltaTime) {
	SDL_FRect overlap;
	if (engine::checkOverlap(obj1.entity, obj2.entity, overlap)) {
		// found intersection, resolve collision
		collisionResponse(state, gs, res, overlap, obj1, obj2, deltaTime);
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="state"></param>
/// <param name="gs"></param>
/// <param name="res"></param>
void createtiles(const SDLState& state, GameState& gs, Resources& res) {
	/*
		0 = Empty
		1 = Ground
		2 = Panel
		3 = Enemy
		4 - Player
		5 - Grass
		6 - Brick
	*/
	short map[MAP_ROWS][MAP_COLS] = {
		0, 0, 2, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 0, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
		2, 4, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 3, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,
		1, 1, 1, 1, 1, 6, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	};

	short foreground[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	short background[MAP_ROWS][MAP_COLS] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		6, 0, 6, 6, 6, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 6, 6, 6, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 6, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS])
		{
			const auto createObject = [&](int r, int c, ObjectType type, SDL_Texture* texture) {
				GameObject obj;
				obj.type = type;
				obj.entity.texture = texture;
				obj.entity.collider = SDL_FRect{ 0, 0, TILE_SIZE, TILE_SIZE };
				obj.entity.position = glm::vec2(
					c * TILE_SIZE,
					state.logH - (MAP_ROWS - r) * TILE_SIZE // bottom of screen minus the row offset times tile size
				);
				return obj;
			};

			for (int r = 0; r < MAP_ROWS; r++) {
				for (int c = 0; c < MAP_COLS; c++) {
					switch (layer[r][c]) {
						case 1: // Ground
						{
							GameObject ground = createObject(r, c, ObjectType::level, res.texGround);
							gs.layers[LAYER_IDX_LEVEL].push_back(ground);
							break;
						}
						case 2: // Panel
						{
							GameObject panel = createObject(r, c, ObjectType::level, res.texPanel);
							gs.layers[LAYER_IDX_LEVEL].push_back(panel);
							break;
						}
						case 3:
						{
							GameObject enemy = createObject(r, c, ObjectType::Enemy, res.texEnemy);
							enemy.entity.currentAnimation = res.ANUM_ENEMY;
							enemy.entity.animations = res.enemyAnimations;
							enemy.entity.collider = SDL_FRect{
								.x = 10, .y = 4, .w = 12, .h = 28
							};
							gs.layers[LAYER_IDX_CHARACTERS].push_back(enemy);
							break;
						}
						case 4: // Player
						{
							GameObject player = createObject(r, c, ObjectType::Player, res.texIdle);
							player.data.player = PlayerData();
							player.entity.animations = res.animations;
							player.entity.currentAnimation = res.ANUM_PLAYER_IDLE;
							player.entity.acceleration = glm::vec2(200.0f, 0.0f);
							player.entity.maxSpeed.x = 100.0f;
							player.entity.dynamic = true;
							player.entity.collider = SDL_FRect{ 11, 6, 10, 26 };
							gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
							gs.playerIdx = static_cast<int>(gs.layers[LAYER_IDX_CHARACTERS].size()) - 1;
							break;
						}
						case 5: // Grass
						{
							GameObject grass = createObject(r, c, ObjectType::level, res.texGrass);
							gs.foregroundTiles.push_back(grass);
							break;
						}
						case 6: // Brick
						{
							GameObject brick = createObject(r, c, ObjectType::level, res.texBrick);
							gs.backgroundTiles.push_back(brick);
							break;
						}
					default:
						break;
					}
				}
			}
		};
	

	loadMap(map);
	loadMap(background);
	loadMap(foreground);
	assert(gs.playerIdx != -1 && "Player not created in map");
}

void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool isPressed) 
{

	const float jumpImpulse = -200.0f; // jump impulse

	if (obj.type == ObjectType::Player) {
		switch (obj.data.player.state) {
			case PlayerState::idle:
			{
				if (key == SDL_SCANCODE_SPACE && isPressed) {
					obj.entity.velocity.y += jumpImpulse;
					obj.data.player.state = PlayerState::jumping;
				}
				break;
			}
			case PlayerState::running:
			{
				if (key == SDL_SCANCODE_SPACE && isPressed) {
					obj.entity.velocity.y += jumpImpulse;
					obj.data.player.state = PlayerState::jumping;
				}
				break;
			}
		}
	}
}

void DrawParalaxBackground(SDL_Renderer* renderer, SDL_Texture* texture,
	float xVelocity, float& scrollPos, float scrollFactor, float deltaTime)
{
	scrollPos -= xVelocity * scrollFactor * deltaTime;
	if (scrollPos <= -texture->w)
	{
		scrollPos = 0;
	}

	SDL_FRect dst{
		.x = scrollPos, .y = 10,
		.w = texture->w * 2.0f,
		.h = static_cast<float>(texture->h)
	};

	SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}
