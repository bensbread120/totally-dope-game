#pragma once
#include "engine/entity.h"
#include <cstdint>

// Octal movement allows moving to diagonal tiles. Ordinal directions are
// purposefully a union of cardinal directions so movement checks on cardinal
// directions alone are possible, e.g. player movement == up will return true
// if player is moving UpRight or UpLeft.
enum class Direction : uint8_t {
	None = 0,
	Up = 1 << 0,
	Down = 1 << 1,
	Left = 1 << 2,
	Right = 1 << 3,
	UpLeft = Up | Left,
	UpRight = Up | Right,
	DownLeft = Down | Left,
	DownRight = Down | Right,
};
enum class MovementState { Idle, Walking };

inline glm::vec2 directionVector(Direction dir)
{
	switch (dir)
	{
		case Direction::Up:    return glm::vec2(0.0f, -1.0f);
		case Direction::Down:  return glm::vec2(0.0f, 1.0f);
		case Direction::Left:  return glm::vec2(-1.0f, 0.0f);
		case Direction::Right: return glm::vec2(1.0f, 0.0f);
	}
	return glm::vec2(0.0f, 1.0f);
}

struct PlayerData
{
	MovementState state = MovementState::Idle;
	Direction direction = Direction::Down;
};

struct EnemyData
{
	MovementState state = MovementState::Idle;
	Direction direction = Direction::Down;
	Timer wanderTimer;
	EnemyData() : wanderTimer(1.5f) {}
};

struct WallData {};

union ObjectData
{
	PlayerData player;
	EnemyData enemy;
	WallData wall;

	// EnemyData has a user-defined default constructor (for wanderTimer),
	// so -- same reason as the platformer -- the union needs an explicit
	// default constructor of its own; the compiler can't pick a member for
	// you once one of them isn't trivial to construct.
	ObjectData() : wall{} {}
};

enum class ObjectType { Player, Enemy, Wall };

struct GameObject
{
	engine::Entity entity;
	ObjectType type = ObjectType::Wall;
	ObjectData data;
};

// Floor tiles are purely decorative: they never move, animate, or collide,
// so they don't need the full weight of engine::Entity.
struct TileSprite
{
	glm::vec2 position;
	SDL_Texture* texture;
};
