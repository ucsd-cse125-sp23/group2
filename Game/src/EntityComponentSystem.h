#pragma once
#include <vector>
#include <queue>

#include <array>
#include "graphics/core.h"
#include "GameConstants.h"
#include <math.h>

//Entity type
using Entity = uint32_t;

//Define Component Tpyes (Max Components = 32)
using Active = bool; //Is Entity active in the scene?
using Position = glm::vec3; //Entity Position in 3D Space
using Velocity = glm::vec3; //Entity Velocity in 3D Space

struct PathData //Data for entity pathing
{
    glm::vec3 pathNodes[PATH_LENGTH]; //3D positions in which entity will need to change direction
    int currentNode; //Index of current node that entity is pathing towards
    float moveSpeed; //distance enemy covers in 1 server tick
};

struct Model //3D Model to render for the entity
{
    int modelID;
    char asciiRep;
    //TODO: Other Model Data
};


struct HitpointData //Data for keeping track of enemy health
{
    int maxHP; //Used to render health bars (scale = HP/maxHP)
    int HP; //Entity health (Hitpoints)
};

struct Turret //Component of Towers
{
    float range; //The range of the tower
    int damage; //The damage that the turret deals per tick
};

struct Collider //Information for collisions
{
    glm::vec3 AABB; //Axis Aligned Bound Box vector

    //TODO: Pointer to a mesh for narrow phase
};

struct RigidBodyInfo //Information for collisions
{
    bool fixed;
    int mass;
};

struct CollisionEvent {
    Entity e;
    Entity o;
    glm::vec3 pen;

};

//Define Component Tags
using Tag = uint32_t;
namespace ComponentTags
{
    constexpr Tag Active            = 0x1;
    constexpr Tag Position          = 0x1 << 1;
    constexpr Tag Velocity          = 0x1 << 2;
    constexpr Tag PathData          = 0x1 << 3;
    constexpr Tag Model             = 0x1 << 4;
    constexpr Tag Collidable        = 0x1 << 5;
    constexpr Tag DiesOnCollision   = 0x1 << 6;
    constexpr Tag RigidBody         = 0x1 << 7;
    constexpr Tag HitpointData      = 0x1 << 8;
    constexpr Tag Turret            = 0x1 << 10;

}

namespace GameData
{
    //List of entities index by entity ID
    //Entity Tag is a 32 bit int that denotes the components attached to the enitity
    extern std::array<Tag, MAX_ENTITIES> tags;

    extern std::array<Active, MAX_ENTITIES> activity;
    extern std::array<Position, MAX_ENTITIES> positions;
    extern std::array<Velocity, MAX_ENTITIES> velocities;
    extern std::array<PathData, MAX_ENTITIES> pathStructs;
    extern std::array<Model, MAX_ENTITIES> models;
    extern std::array<HitpointData, MAX_ENTITIES> hitpointStructs;
    extern std::array<Turret, MAX_ENTITIES> turrets;
    extern std::array<Collider, MAX_ENTITIES> colliders;
    extern std::array<RigidBodyInfo, MAX_ENTITIES> rigidbodies;


    //Events
    extern std::queue<CollisionEvent> colevents;
}

namespace EntityComponentSystem
{

    //Contains Calls to ALL systems
    void update();

    //Move entities that have a velocity component
    void sysMovement();

    //Do pathfinding for entities that have a path component
    void sysPathing();

    //Detect Collisions
    void sysDetectCollisions();

    //Handle&Resolve Collisions
    void resolveCollisions();


    //All automated turret / tower firing
    void sysTurretFire();

    //Check the status of entity's HP
    void sysHealthStatus();
};
