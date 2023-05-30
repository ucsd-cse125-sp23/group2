#include "ServerGame.h"

ServerGame::ServerGame(void)
{
    std::srand(std::time(nullptr));
    currentStatus = init;
    // set up the server network to listen
    network = new ServerNetwork();

    initializeGame();

    debug[0] = '\0';

    curTick = 0;
    clientsConnected = 0;
}

//Populate Component Arrays
void ServerGame::initializeGame()
{
    initPlayers();
    initPrefabs();
    initBase();
    initResources();
    initWaves();
}

void ServerGame::initPlayers()
{
    prefabMap[Prefab::Players]();

}

void ServerGame::initWaves()
{
    WaveData::currentWave = -1;
    WaveData::waveTick = ENEMY_SPAWNDELAY_TICKS;

    //Temp Nested for loop to populate wave vectors
    for (int i = 0; i < WAVE_COUNT; i++)
    {
        for (int j = 0; j < 15; j++)
        {
            enemy e = { WaveData::enemyTypes[rand() % NUM_ENEMY_TYPES], rand() % Paths::pathCount, 1 * TICK_RATE };
            WaveData::waves[i].push(e);
        }
    }
}

void ServerGame::initBase()
{
    home = prefabMap[Prefabs::Home]().front();
}

void ServerGame::waveSpawner()
{
    static int spawnCooldown = 0;

    if (WaveData::waveTick <= 0)
    {
        WaveData::currentWave++;
        if (WaveData::currentWave < WAVE_COUNT)
        {
            WaveData::waveTick = WaveData::waveTimers[WaveData::currentWave];
            spawnCooldown = WaveData::waves[WaveData::currentWave].front().cooldown;
        }
    }

    if (WaveData::currentWave >= 0 && WaveData::currentWave < WAVE_COUNT)
    {
        if (!WaveData::waves[WaveData::currentWave].empty())
        {
            if (spawnCooldown <= 0)
            {
                list<Entity> e = prefabMap[WaveData::waves[WaveData::currentWave].front().id]();
                if (e.front() != INVALID_ENTITY)
                {
                    GameData::pathStructs[e.front()].path = WaveData::waves[WaveData::currentWave].front().path;
                    GameData::positions[e.front()] = Paths::path[GameData::pathStructs[e.front()].path][0];
                    Collision::updateColTable(e.front());
                    WaveData::waves[WaveData::currentWave].pop();
                    if (!WaveData::waves[WaveData::currentWave].empty())
                    {
                        spawnCooldown = WaveData::waves[WaveData::currentWave].front().cooldown;
                    }
                }
            }
            spawnCooldown--;
        }
    }

    WaveData::waveTick--;
}

//Spawn Initial assortment of resources
void ServerGame::initResources()
{
    std::vector<glm::vec3> positions = PoissonDisk::genPoints();

    printf("Number of resources %d", positions.size());

    std::vector<Entity> resources = std::vector<Entity>();

    for (glm::vec3 pos : positions) {
        Entity e;
        if(std::rand() > RAND_MAX/2)
            e = prefabMap[Prefabs::BASIC_STONE_RESOURCE]().front();
        else
            e = prefabMap[Prefabs::BASIC_WOOD_RESOURCE]().front();
        if (e != INVALID_ENTITY) {
            GameData::positions[e] = pos - glm::vec3(WORLD_X/2, 0, WORLD_Z/2);
            GameData::tags[e] |= ComponentTags::DiesOnCollision;
            GameData::colliders[e].colwith |= CollisionLayer::UIObj + CollisionLayer::StaticObj;
            Collision::updateColTable(e);
            resources.push_back(e);
        }
        //printf("Pos is %f, %f, %f\n", pos.x, pos.y, pos.z);
    }
    for (Entity e : resources) {
        for (Entity p : Paths::pathlist) {
            ECS::colCheck(e, p);
        }
        ECS::colCheck(e, MAX_ENTITIES_NOBASE);
        ECS::resolveCollisions();
        GameData::tags[e] ^= ComponentTags::DiesOnCollision;
        GameData::colliders[e].colwith ^= CollisionLayer::UIObj + CollisionLayer::StaticObj;
    }


}

// Update function called every tick
void ServerGame::update()
{
    // get new clients
    if (network->acceptNewClient())
    {
        clientsConnected++;
        printf("Client %u connected\n", clientsConnected);
    }
    //Receve Input
    receiveFromClients();
    //auto startTime = std::chrono::steady_clock::now();
    //auto endTime = std::chrono::steady_clock::now();
    //auto duration = std::chrono::duration<double, std::milli>(endTime - startTime);
    switch (currentStatus){
    case init:
        if (clientsConnected >= NUM_PLAYERS) {
            currentStatus = game;
            printf("All players connected, starting main update loop!\n");
        }
        break;
    case game:
        GameData::slogpos = 0;

        handleInputs();
        //startTime = std::chrono::steady_clock::now();
        EntityComponentSystem::update();
        //endTime = std::chrono::steady_clock::now();
        //duration = std::chrono::duration<double, std::milli>(endTime - startTime);
        //std::cout << "ECS took " << duration.count() << " milliseconds.\n";

        waveSpawner();

        checkStatus();

        if (curTick % 4 == 0) {
            //asciiView();
        }
        curTick++;
        break;
    case loss:
        break;
    case win:
        break;
    default:
        printf("Invalid server state!");
    }




    //Print debug message buffer
    printf(debug);
    debug[0] = '\0';


}

void ServerGame::handleInputs()
{

    char msg[100];
    msg[0] = '\0';

    for(int i = 0; i < NUM_CLIENTS; i++)
    {
        glm::vec3 camDirection;
        glm::vec3 camPosition;
        bool target = false;
        while (!incomingDataLists[i].empty())
        {
            if ((GameData::tags[i] & ComponentTags::Dead) == ComponentTags::Dead) {
                std::queue<ClienttoServerData> empty;
                std::swap(incomingDataLists[i], empty);
                continue;
            }
            ClienttoServerData in = incomingDataLists[i].front();
            GameData::velocities[i].velocity = glm::vec3(0,GameData::velocities[i].velocity.y,0);
            camDirection = in.camDirectionVector;
            camPosition = in.camPosition;
            if ( ((in.moveLeft ^ in.moveRight)) || ((in.moveForward ^ in.moveBack))) {
                float camAngle = in.camAngleAroundPlayer;
                glm::vec3 moveDirection = glm::normalize(glm::rotate(glm::radians(camAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::normalize(glm::vec4(in.moveLeft - in.moveRight, 0.0f, in.moveForward - in.moveBack, 0.0f)));
                GameData::models[i].modelOrientation = -camAngle + glm::degrees(glm::acos(moveDirection.y));
                GameData::velocities[i].velocity += PLAYER_MVSPD * moveDirection;
            }
            if (in.shoot) {
                target = in.shoot;
            }

            if (in.build) {
                changeState(i, PlayerState::Build);
            }
            else {
                changeState(i, PlayerState::Default); //May be slow
                if (GameData::retplaces[i].reticle != INVALID_ENTITY) {
                    ECS::causeDeath(GameData::retplaces[i].reticle, GameData::retplaces[i].reticle);
                    GameData::retplaces[i].reticle = INVALID_ENTITY;
                }
            }

            if (in.jump && GameData::rigidbodies[i].grounded) {
                GameData::velocities[i].velocity.y = PLAYER_JPSPD;
                // Add jumping sound to sound log
                GameData::soundLogs[GameData::slogpos].source = i;
                GameData::soundLogs[GameData::slogpos].sound = SOUND_ID_JUMP;
                GameData::slogpos++;
            }
            incomingDataLists[i].pop();
        }

        if (GameData::states[i] == PlayerState::Build) {
            //printf("Calling Player build\n");
            playerBuild(i, camDirection, camPosition, TOWER_PLACEMENT_RANGE);
        }

        if (target) {
            if (GameData::states[i] == PlayerState::Build) {
                GameData::retplaces[i].place = true;
            }
            else {
                changeState(i, PlayerState::Attack);
                playerAttack(i, camDirection, camPosition);
            }
            //printf("ShootingInput\n");
        }
        else if(GameData::states[i] == PlayerState::Attack){
            changeState(i, PlayerState::Default);
        }
        //in.print(msg);
    }

}


void ServerGame::sendPackets()
{
    if (currentStatus == game) {
        //Send Data to Clients
        packageData(gameState);
        network->sendActionPackets(gameState);
    }
}

void ServerGame::initPrefabs()
{
    Paths::pathlist = prefabMap[Prefabs::PathColliders]();
    /*
    for (Entity e : pathlist) {
        if (e != INVALID_ENTITY) {
            printf("Collider for ent %d, is (%f, %f, %f)\n", e, GameData::colliders[e].AABB.x, GameData::colliders[e].AABB.y, GameData::colliders[e].AABB.z);
            printf("Position for ent %d, is (%f, %f, %f)\n", e, GameData::positions[e].x, GameData::positions[e].y, GameData::positions[e].z);
            if (ComponentTags::Collidable & GameData::tags[e]) {
                printf("can Collide\n");
            }
            if (GameData::activity[e]) {
                printf("is active\n");
            }
        }
    }
    */
    //Init Path Objects

}

void ServerGame::receiveFromClients()
{

    network->receiveDeserialize(incomingDataLists);

}

void ServerGame::packageData(ServertoClientData& data)
{
    data.positions = GameData::positions;
    data.models = GameData::models;
    data.activity = GameData::activity;
    data.healths = GameData::healths;
    data.combatLogs = GameData::combatLogs;
    data.clogsize = GameData::clogpos;
    data.soundLogs = GameData::soundLogs;
    data.slogsize = GameData::slogpos;
    data.currentWave = WaveData::currentWave + 1;
    data.numWaves = WAVE_COUNT;
    data.playerData = GameData::playerdata;
    data.buildcosts = buildcosts;
    data.serverStatus = currentStatus;
    data.colliders = GameData::colliders;
    data.waveTimer = WaveData::waveTick / TICK_RATE;
}

const int GRID_X = 32;
const int GRID_Z = 32;
void ServerGame::asciiView() {
    // "Clear" the screen
    printf("\033[2J");
    printf("\033[%d;%dH", 0, 0);
    // Create "grid"
    char grid[GRID_X][GRID_Z];
    // Initialize it with all periods
    memset(grid, '.', sizeof(grid));
    for (Entity e = 0; e < MAX_ENTITIES; e++) {
        // Continue to next entity if this one is not active
        if (!GameData::activity[e]) { continue; }
        // Continue to next entity if this one doesn't have position
        if ((GameData::tags[e] & ComponentTags::Position) != ComponentTags::Position) { continue; }
        // Continue to next entity if this one doesn't have model
        if ((GameData::tags[e] & ComponentTags::Model) != ComponentTags::Model) { continue; }
        // Floor entity position
        int xCoord = floor(GameData::positions[e].x);
        int zCoord = floor(GameData::positions[e].z);
        // If its within grid bounds, add to grid
        if (xCoord >= 0 && xCoord < GRID_X && zCoord >= 0 && zCoord < GRID_Z) {
            grid[xCoord][zCoord] = GameData::models[e].asciiRep;
        }
    }
    // Print the grid
    for (int i = 0; i < GRID_X; ++i) {
        for (int j = 0; j < GRID_Z; ++j) {
            cout << grid[i][j];
        }
        cout << endl;
    }
}

void ServerGame::playerAttack(Entity e, glm::vec3& camdir, glm::vec3& campos)
{
    GameData::pattackmodules[e].targetPos = ECS::computeRaycast(campos, camdir, glm::distance(campos, GameData::positions[e])+glm::length(GameData::colliders[e].AABB), FLT_MAX);
    //printf("Targer pos (%f, %f, %f)\n", GameData::pattackmodules[e].targetPos.x, GameData::pattackmodules[e].targetPos.y, GameData::pattackmodules[e].targetPos.z);
}

void ServerGame::changeState(Entity e, State post)
{
    GameData::tags[e] ^= GameData::states[e];
    GameData::states[e] = post;
    GameData::tags[e] |= post;
}

void ServerGame::playerBuild(Entity i, glm::vec3& camdir, glm::vec3& campos, float range)
{
    if (camdir.y >= 0) {
        //printf("You're looking up\n");
        GameData::retplaces[i].validTarget = false;
        return;
    }
    glm::vec3 dirYNorm = camdir / (camdir.y*-1);
    glm::vec3 targetpos = glm::vec3(campos.x + dirYNorm.x * campos.y, 0, campos.z + dirYNorm.z * campos.y);
    if (glm::distance(targetpos, GameData::positions[i]) > range) {
        //printf("Out of range\n");

        GameData::retplaces[i].validTarget = false;
        return;
    }
    GameData::retplaces[i].targetPos = targetpos;
    GameData::retplaces[i].validTarget = true;
    //printf("Valid Target\n");
}

void ServerGame::checkStatus() {
    if (WaveData::currentWave == WAVE_COUNT && WaveData::waveTick <= 0) {
        printf("WIN! :D\n");
        currentStatus = win;
    }
    if (!GameData::activity[home]) {
        printf("LOSE! :(\n");
        currentStatus = loss;
    }
}
