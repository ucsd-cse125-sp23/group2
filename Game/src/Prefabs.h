#pragma once
#include <list>
#include <queue>
#include "graphics/core.h"
#include "GameConstants.h"

/*
* The default axis is negative z is the towards the target
* Positive x is right, negative x is left
* Positve y is up
* negative y is down
* Unit scale is the same
* Originates at the origin
* (Must transform, positions and velocities to make use of prefabs)
*
*/
using Entity = uint32_t;

std::list<Entity> createProjectileBasic();
std::list<Entity> createProjectileSpread5();
std::list<Entity> createProjectileChaos();
std::list<Entity> createProjectileRandom();
std::list<Entity> createEnemyGroundBasic();
std::list<Entity> createTowerReticle();
std::list<Entity> createTowerBasic();
std::list<Entity> createHome();

const int NUM_PREFABS = 32;
using Prefab = uint32_t;
typedef std::list<Entity>(*PrefabFunction)(); // function pointer type
const PrefabFunction prefabMap[NUM_PREFABS] = {
	&createProjectileBasic,
	&createProjectileSpread5,
	&createProjectileChaos,
	&createProjectileRandom,
	&createEnemyGroundBasic,
	&createTowerReticle,
	&createTowerBasic,
	&createHome
};

namespace Prefabs {
	constexpr Prefab ProjectileBasic = 0;
	constexpr Prefab ProjectileSpread5 = 1;
	constexpr Prefab ProjectileChaos = 2;
	constexpr Prefab ProjectileRandom = 3;
	constexpr Prefab EnemyGroundBasic = 4;
	constexpr Prefab TowerReticle = 5;
	constexpr Prefab TowerBasic = 6;
	constexpr Prefab Home = 7;
};

namespace Paths {
	extern int const pathCount;
	extern glm::vec3 path[4][PATH_LENGTH];
};

struct enemy {
	int id;
	int path;
	int cooldown;
};

namespace WaveData {

	extern int currentWave;

	extern int waveTimers[WAVE_COUNT]; //Time before starting wave

	extern int waveTick; //countdown timer for waves
	
	extern std::queue<enemy> waves[WAVE_COUNT];
}