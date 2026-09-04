// sdl3-demo.cpp : Defines the entry point for the application.
//
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include "sdl3-demo.h"
#include "gameobject.h"
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
	int playerIdx;

	GameState() {
		playerIdx = -1; // changed when loaded
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
	std::vector<Animation> animations;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, *texRunning, *texBrick, *texGrass, *texGround, *texPanel;

	SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& filepath)
	{
		SDL_Log("Loading texture: %s", filepath.c_str());

		SDL_Texture* tex = IMG_LoadTexture(renderer, filepath.c_str());

		if (!tex)
		{
			SDL_Log("FAILED: %s", SDL_GetError());
			return nullptr;
		}

		SDL_Log("SUCCESS: %s", filepath.c_str());

		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);

		return tex;
	}

	void load(SDLState& state)
	{
		SDL_Log("Base path: %s", SDL_GetBasePath());
		SDL_Log("Current directory: %s", SDL_GetCurrentDirectory());
		animations.resize(5);
		animations[ANUM_PLAYER_IDLE] = Animation(8, 1.6f); // 8 frames, 1.6 second long
		animations[ANUM_PLAYER_RUNNING] = Animation(4, 0.5f); // 4 frames, 0.5 second long

		texIdle = loadTexture(state.renderer, "assets/idle.png");
		texRunning = loadTexture(state.renderer, "assets/run.png");
		texBrick = loadTexture(state.renderer, "assets/brick.png");
		texGrass = loadTexture(state.renderer, "assets/grass.png");
		texGround = loadTexture(state.renderer, "assets/ground.png");
		texPanel = loadTexture(state.renderer, "assets/panel.png");
	}

	void unload() {
		for (SDL_Texture* tex : textures) {
			SDL_DestroyTexture(tex);
		}
		textures.clear();
	}
};

bool initialise(SDLState& state);
void cleanup(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createtiles(const SDLState& state, GameState& gs, Resources& res);
void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& obj1, GameObject& obj2, float deltaTime);
void collisionResponse(const SDLState& state, GameState& gs, Resources& res,
	const SDL_FRect& rect1, const SDL_FRect& rect2, const SDL_FRect& rect3,
	GameObject& obj1, GameObject& obj2, float deltaTime);
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool isPressed);

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
	GameState gs;
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
					break;
				}
			}
		}
		// update all game objects
		for (auto& layer : gs.layers) {
			for (GameObject& obj : layer) {
				update(state, gs, resources, obj, deltaTime);
				// update animations
				if (obj.currentAnimation != -1) 
				{
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}

		// perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 55, 55, 255);
		SDL_RenderClear(state.renderer);

		// draw all objects
		for (auto& layer : gs.layers) {
			for (GameObject &obj : layer) {
				drawObject(state, gs, obj, deltaTime);
			}
		}

		// debug section
		SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
		SDL_RenderDebugText(state.renderer, 10, 10, std::format("State: {}", static_cast<int>(gs.getPlayer().data.player.state)).c_str());

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
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime) {

	const float spriteSize = 32;
	float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * spriteSize : 0.0f;
	SDL_FRect src{
		.x = srcX,
		.y = 0,
		.w = spriteSize,
		.h = spriteSize
	};

	SDL_FRect dst{
		.x = obj.position.x,
		.y = obj.position.y,
		.w = spriteSize,
		.h = spriteSize
	};

	SDL_FlipMode flipMode = (obj.direction < 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
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
	if (obj.dynamic) {
		// apply gravity
		obj.velocity += glm::vec2(0, 500) * deltaTime;
	}
	

	if (obj.type == ObjectType::Player) {
		// Update player-specific logic
		float currentDirection = 0;
		if (state.keys[SDL_SCANCODE_LEFT]) {
			currentDirection -= 1.0f;
		}
		if (state.keys[SDL_SCANCODE_RIGHT]) {
			currentDirection += 1.0f;
		}
		if (state.keys[SDL_SCANCODE_ESCAPE]) {
			// exit game
		}
		if (currentDirection)
		{
			obj.direction = currentDirection;
		}

		switch (obj.data.player.state)
		{
			case PlayerState::idle:
			{
				if (currentDirection)
				{
					obj.data.player.state = PlayerState::running;
					obj.texture = res.texRunning;
					obj.currentAnimation = res.ANUM_PLAYER_RUNNING;
				}
				else {
					// deccelerate to stop
					if (obj.velocity.x)
					{
						const float decceeration = obj.velocity.x > 0 ? -1.5f : 1.5f;
						float amount = decceeration * obj.acceleration.x * deltaTime;
						if (std::abs(amount) > std::abs(obj.velocity.x)) 
						{
							obj.velocity.x = 0;
						}
						else {
							obj.velocity.x += amount;
						}
					}
				}
				break;
			}
			case PlayerState::running:
			{
				if (!currentDirection)
				{
					obj.data.player.state = PlayerState::idle;
					obj.texture = res.texIdle;
					obj.currentAnimation = res.ANUM_PLAYER_IDLE;
				}
				break;
			}
		}

		// add acceleration to velocity
		obj.velocity += currentDirection * obj.acceleration * deltaTime;
		if (std::abs(obj.velocity.x) > obj.maxSpeedX) {
			obj.velocity.x = currentDirection * obj.maxSpeedX;
		}

		
	}
	// add velocity to position
	obj.position += obj.velocity * deltaTime;
	bool foundGround = false;
	for (auto& layer : gs.layers) {
		for (GameObject& other : layer) {
			if (&obj != &other) {
				checkCollision(state, gs, res, obj, other, deltaTime);

				// check if player is grounded
				SDL_FRect sensor{
					.x = obj.position.x + obj.collider.x,
					.y = obj.position.y + obj.collider.y + obj.collider.h,
					.w = obj.collider.w,
					.h = 1 // 1 pixel height sensor
				};
				SDL_FRect rectB{
					.x = other.position.x + other.collider.x,
					.y = other.position.y + other.collider.y,
					.w = other.collider.w, .h = other.collider.h

				};
				if (SDL_HasRectIntersectionFloat(&sensor, &rectB)) {
					foundGround = true;
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

void collisionResponse(const SDLState& state, GameState& gs, Resources& res,
	const SDL_FRect& rect1, const SDL_FRect& rect2, const SDL_FRect& rect3,
	GameObject& obj1, GameObject& obj2, float deltaTime) 
{
	if (obj1.type == ObjectType::Player)
	{
		switch (obj2.type)
		{
			case (ObjectType::level):
			{
				// resolve collision with level object
				if (rect3.w < rect3.h) {
					// horizontal collision
					if (obj1.velocity.x > 0) {
						// obj1 is moving right
						obj1.position.x -= rect3.w;
					}
					else if (obj1.velocity.x < 0) {
						// obj1 is moving left
						obj1.position.x += rect3.w;
					}
					obj1.velocity.x = 0;
				}
				else {
					// vertical collision
					if (obj1.velocity.y > 0) {
						// obj1 is moving down
						obj1.position.y -= rect3.h;
					}
					else if (obj1.velocity.y < 0) {
						// obj1 is moving up
						obj1.position.y += rect3.h;
					}
					obj1.velocity.y = 0;
				}
				break;
			}
		}
	}
}

void checkCollision(const SDLState& state, GameState& gs, Resources& res, GameObject& obj1, GameObject& obj2, float deltaTime) {
	SDL_FRect rect1{
		.x = obj1.position.x + obj1.collider.x, .y = obj1.position.y + obj1.collider.y,
		.w = obj1.collider.w, .h = obj1.collider.h
	};
	SDL_FRect rect2{
		.x = obj2.position.x + obj2.collider.x, .y = obj2.position.y + obj2.collider.y,
		.w = obj2.collider.w, .h = obj2.collider.h
	};

	SDL_FRect rect3{ 0 };
	
	if (SDL_GetRectIntersectionFloat(&rect1, &rect2, &rect3)) {
		// found intersection, resolve collision
		collisionResponse(state, gs, res, rect1, rect2, rect3, obj1, obj2, deltaTime);
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
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		4, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		2, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	const auto createObject = [&](int r, int c, ObjectType type, SDL_Texture* texture) {
		GameObject obj;
		obj.type = type;
		obj.texture = texture;
		obj.collider = SDL_FRect{ 0, 0, TILE_SIZE, TILE_SIZE };
		obj.position = glm::vec2(
			c * TILE_SIZE,
			state.logH - (MAP_ROWS - r) * TILE_SIZE // bottom of screen minus the row offset times tile size
		);
		return obj;
		};

	for (int r = 0; r < MAP_ROWS; r++) {
		for (int c = 0; c < MAP_COLS; c++) {
			switch (map[r][c]) {
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
				case 4: // Player
				{
					GameObject player = createObject(r, c, ObjectType::Player, res.texIdle);
					player.data.player = PlayerData();
					player.animations = res.animations;
					player.currentAnimation = res.ANUM_PLAYER_IDLE;
					player.acceleration = glm::vec2(200.0f, 0.0f);
					player.maxSpeedX = 100.0f;
					player.dynamic = true;
					player.collider = SDL_FRect{ 11, 6, 10, 26 };
					gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
					gs.playerIdx = static_cast<int>(gs.layers[LAYER_IDX_CHARACTERS].size()) - 1;
					break;
				}
				case 5: // Grass
				{
					GameObject grass = createObject(r, c, ObjectType::level, res.texGrass);
					gs.layers[LAYER_IDX_LEVEL].push_back(grass);
					break;
				} 
				case 6: // Brick
				{
					GameObject brick = createObject(r, c, ObjectType::level, res.texBrick);
					gs.layers[LAYER_IDX_LEVEL].push_back(brick);
					break;
				}
				default:
					break;
			}
		}
	}
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
					obj.velocity.y += jumpImpulse;
					obj.data.player.state = PlayerState::jumping;
				}
				break;
			}
			case PlayerState::running:
			{
				if (key == SDL_SCANCODE_SPACE && isPressed) {
					obj.velocity.y += jumpImpulse;
					obj.data.player.state = PlayerState::jumping;
				}
				break;
			}
		}
	}
}