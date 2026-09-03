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

using namespace std;

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

struct GameState
{
	std::array<std::vector<GameObject>, 2> layers;
	int playerIdx;

	GameState() {
		playerIdx = 0; // changed when loaded
	}
};

struct Resources
{
	const int ANUM_PLAYER_IDLE = 0;
	const int ANUM_PLAYER_RUNNING = 1;
	std::vector<Animation> animations;

	std::vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, *texRunning;

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
	GameObject player;
	player.type = ObjectType::Player;
	player.data.player = PlayerData();
	player.texture = resources.texIdle;
	player.animations = resources.animations;
	player.currentAnimation = resources.ANUM_PLAYER_IDLE;
	player.acceleration = glm::vec2(200.0f, 0.0f);
	player.maxSpeedX = 100.0f;
	gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

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

void cleanup(SDLState &state)
{
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}

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

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) 
{
	if (obj.type == ObjectType::Player) {
		// Update player-specific logic
		float currentDirection = 0;
		if (state.keys[SDL_SCANCODE_LEFT]) {
			currentDirection -= 1.0f;
		}
		if (state.keys[SDL_SCANCODE_RIGHT]) {
			currentDirection += 1.0f;
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

		// add velocity to position
		obj.position += obj.velocity * deltaTime;
	}
}