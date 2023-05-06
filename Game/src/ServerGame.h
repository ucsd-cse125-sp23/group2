#pragma once
#include "ServerNetwork.h"
#include <vector>
#include <queue>
#include <array>
#include "EntityComponentSystem.h"
#include "GameConstants.h"
#include "Prefabs.h"
#define DEBUG_BUFFER 100000
namespace ECS = EntityComponentSystem;
class ServerGame
{

public:

    queue<ClienttoServerData> incomingDataLists[NUM_CLIENTS];

    ServertoClientData gameState;

    ServerGame(void);
    ~ServerGame(void);

    void receiveFromClients();

    void packageData(ServertoClientData& data);

    void initializeGame();

    void initPlayers();

    void initWaves();

    void initEnemies();

    void initTowers();

    void initResources();

    void initProjectiles();

    void handleInputs();

    void update();

    void waveSpawner();

    void sendPackets();

    //TODO: Remove this method after testing
    void testing_staggeredSpawn();

    //Create Enemy (Temp function) replace with load from prefab
    Entity createEnemy();
private:

    // The ServerNetwork object 
    ServerNetwork* network;

    //Debug Buffer
    char debug[DEBUG_BUFFER];

    void asciiView();

    unsigned int curTick;

    void playerAttack(Entity i, glm::vec3& camdir, glm::vec3& campos);
};