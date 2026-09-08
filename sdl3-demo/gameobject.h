#pragma once
#include "engine/entity.h"

enum class PlayerState
{
	idle, running, jumping
};

enum class BulletState
{
	moving, colliding, inactive
};

enum class EnemyState
{
	shambling, damaged, dead
};

struct PlayerData
{
	PlayerState state = PlayerState::idle;
	Timer weaponTimer;
	PlayerData() : weaponTimer(0.1f) {}
};

struct LevelData {};

struct EnemyData
{
	EnemyState state = EnemyState::shambling;
};

struct BulletData
{
	BulletState state = BulletState::moving;
};

union ObjectData
{
	PlayerData player;
	EnemyData enemy;
	LevelData level;
	BulletData bullet;

	// PlayerData has a user-defined default constructor, so the compiler
	// can no longer generate one for the union for free -- it has no way
	// to know which member you mean. This is what your original
	// GameObject() constructor was working around with
	// `data{ .level = LevelData() }`.
	ObjectData() : level{} {}
};

enum class ObjectType
{
	Player,
	Enemy,
	level,
	Bullet
};

// Deliberately still platformer-specific: `type`, `data`, and `grounded`
// only mean something in a side-scroller. The part that's identical
// between this game and the top-down one -- position, velocity, sprite,
// animation, collider -- now lives in engine::Entity, so the top-down
// game's equivalent struct can reuse it instead of copy-pasting it.
struct GameObject
{
	engine::Entity entity;
	ObjectType type = ObjectType::level;
	ObjectData data;
	bool grounded = false;
};
