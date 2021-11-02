#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include <vector>
#include <random>

constexpr int DISPLAY_WIDTH = 1280; // convert to constexpr if they don't change and use upper case
constexpr int DISPLAY_HEIGHT = 720;
constexpr int DISPLAY_SCALE = 1;
constexpr int origin_offset_y = 15;
//constexpr float randomMin = 0;
//constexpr float randomMax = 6.3f;

enum Types
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_ASTEROID,
	TYPE_GEM,
	TYPE_PIECES,
	TYPE_METEOR,
	TYPE_RING,
	TYPE_ATTACHED,
	TYPE_WAITING,
	TYPE_PARTICLES,
};

enum Agent8States
{
	STATE_FLYING = 0,
	STATE_ATTACHED,
	STATE_DEAD,
	STATE_START,
};

struct GameState
{
	Agent8States agentStates = STATE_START;
	int score = 0;
	int startingLevel = 2;
	int gemNumber = startingLevel / 2;
	int gemsSpawned = 0;
};

GameState gameState;

void UpdateRock();
void WrapMovement(GameObject& object);
void UpdateAgent();
void StateAttached();
void StateFlying();
void StateDead();
void StateStart();
void AgentAttached();
void SpawnRocks(int level);
void SpawnMeteors(int level);
void SpawnPieces(GameObject& object);
void UpdatePieces();
void SpawnGems(int id);
void UpdateGems();
void Restart(int level);
void UpdateRings();
void SpawnParticles();
void UpdateParticles();

// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");

	//Spawn Player and set origin for crawling sprites
	int id_agent = Play::CreateGameObject(TYPE_AGENT8, { 0,0 }, 50, "agent8_left_7");
	int agent_height = Play::GetSpriteHeight(Play::GetGameObject(id_agent).spriteId);
	Play::MoveSpriteOrigin("agent8_left_7", 0, agent_height / 2 + origin_offset_y);
	Play::MoveSpriteOrigin("agent8_right_7", 0, agent_height / 2 + origin_offset_y);
	Play::MoveSpriteOrigin("asteroid_2", 0, 1 - origin_offset_y);

	//Platforms and hazards
	SpawnRocks(gameState.startingLevel);
	SpawnMeteors(gameState.startingLevel);
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	Play::DrawBackground();
	//Play::ClearDrawingBuffer(Play::cBlack);
	UpdateRock();
	UpdateAgent();
	UpdatePieces();
	UpdateGems();
	UpdateRings();
	UpdateParticles();

	//Next level
	if (gameState.score == gameState.gemNumber)
	{
		gameState.startingLevel += 1;
		gameState.gemNumber = gameState.startingLevel / 2;
		Play::PlayAudio("reward");
		Restart(gameState.startingLevel);
	}
	Play::PresentDrawingBuffer();
	return Play::KeyDown( VK_ESCAPE );
}

void SpawnRocks(int level)
{
	//Same no. of asteroids as the level
	for (int i = 1; i <= level; i++)
	{
		//Randomly set position and rotation
		int pos_x = Play::RandomRoll(DISPLAY_WIDTH);
		int pos_y = Play::RandomRoll(DISPLAY_HEIGHT);
		int id_rock = Play::CreateGameObject(TYPE_ASTEROID, { pos_x, pos_y }, 60, "asteroid_2");
		
		//std::random_device seed;
		//std::default_random_engine eng(seed());
		//std::uniform_real_distribution<float> randomNumber(randomMin, randomMax);
		//float rotation = randomNumber(eng);
		
		float rotation = (((float)rand()/ RAND_MAX) * (PLAY_PI * 2));	// 0-2PI radians
		GameObject& obj_rock = Play::GetGameObject(id_rock);
		obj_rock.rotation = rotation;
		obj_rock.animSpeed = 0.05;
	}
	std::vector<int> vRocks = Play::CollectGameObjectIDsByType(TYPE_ASTEROID);

	//Randomly choose one for Agent8 to start on
	int id_attached = Play::RandomRollRange(0, vRocks.size() -1);
	GameObject& obj_attached = Play::GetGameObject(vRocks.at(id_attached));
	obj_attached.type = TYPE_ATTACHED;
}

void SpawnMeteors(int level)
{
	//Same as for asteroids but half has many meteors
	for (int i = 1; i <= level / 2; i++)
	{
		//Random Positon
		int pos_x = Play::RandomRoll(DISPLAY_WIDTH);
		int pos_y = Play::RandomRoll(DISPLAY_HEIGHT);
		int id_meteor = Play::CreateGameObject(TYPE_METEOR, { pos_x, pos_y }, 60, "meteor");
		GameObject& obj_meteor = Play::GetGameObject(id_meteor);
		std::vector<int> vRocks = Play::CollectGameObjectIDsByType(TYPE_ASTEROID);
		GameObject& obj_attached = Play::GetGameObjectByType(TYPE_ATTACHED);
		for (int id : vRocks)
		{
			GameObject& obj_rock = Play::GetGameObject(id);
			if (Play::IsColliding(obj_meteor, obj_rock) || Play::IsColliding(obj_meteor, obj_attached))
			{
				obj_meteor.pos.x = obj_meteor.pos.x + 20 * sin(obj_meteor.rotation);
				obj_meteor.pos.y = obj_meteor.pos.y + 20 * -cos(obj_meteor.rotation);
			}
		}

		//Random Float rotation
		/*std::random_device seed;
		std::default_random_engine eng(seed());
		std::uniform_real_distribution<float> randomNumber(randomMin, randomMax);*/

		float rotation = (((float)rand() / RAND_MAX) * (PLAY_PI * 2));	// 0-2PI radians
		obj_meteor.rotation = rotation;
		obj_meteor.animSpeed = 0.05;
	}
}

void UpdateRock()
{
	//For both asteroids and meteors
	std::vector<std::vector<int>> vRocks =
	{
		{ Play::CollectGameObjectIDsByType(TYPE_ASTEROID) },
		{ Play::CollectGameObjectIDsByType(TYPE_METEOR) }
	};
	for (std::vector<int> vec : vRocks)
	{
		for (int id : vec)
		{
			//Movement
			GameObject& obj_rock = Play::GetGameObject(id);
			Play::DrawObjectRotated(obj_rock);
			Play::SetGameObjectDirection(obj_rock, 4, obj_rock.rotation);

			if (Play::IsLeavingDisplayArea(obj_rock))
			{
				WrapMovement(obj_rock);
			}

			Play::UpdateGameObject(obj_rock);
		}
	}
}

void UpdateAgent()
{
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_attached = Play::GetGameObjectByType(TYPE_ATTACHED);

	switch (gameState.agentStates)
	{
		case STATE_FLYING:
			StateFlying();

			break;
		
		case STATE_ATTACHED:
			StateAttached();
			
			break;

		case STATE_DEAD:
			StateDead();

			break;

		case STATE_START:
			StateStart();

			break;
	}

	Play::UpdateGameObject(obj_agent);
	Play::DrawObjectRotated(obj_agent);
	Play::DrawFontText("105px", "Gems = " + std::to_string(gameState.score), { 50,50 }, Play::LEFT);
}

void StateFlying()
{
	//Variables
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vRocks = Play::CollectGameObjectIDsByType(TYPE_ASTEROID);
	std::vector<int> vMeteors = Play::CollectGameObjectIDsByType(TYPE_METEOR);
	std::vector<int> vGems = Play::CollectGameObjectIDsByType(TYPE_GEM); //In case there are multiple gems at once

	//Movement
	Play::SetSprite(obj_agent, "agent8_fly", 1);
	Play::SetGameObjectDirection(obj_agent, 7, obj_agent.rotation);
	SpawnParticles();
	if (Play::KeyDown(VK_RIGHT))
	{
		obj_agent.rotSpeed = 0.05;
	}
	else if (Play::KeyDown(VK_LEFT))
	{
		obj_agent.rotSpeed = -0.05;
	}
	else
	{
		obj_agent.rotSpeed = 0;
	}
	if (Play::IsLeavingDisplayArea(obj_agent))
	{
		WrapMovement(obj_agent);
	}

	//Landing on another asteroid
	for (int id : vRocks)
	{
		GameObject& obj_rock = Play::GetGameObject(id);
		if (Play::IsColliding(obj_agent, obj_rock))
		{
			gameState.agentStates = STATE_ATTACHED;
			Play::SetSprite(obj_agent, "agent8_left_7", 0);
			obj_rock.type = TYPE_ATTACHED;
			Play::PointGameObject(obj_agent, 0, obj_agent.oldPos.x, obj_agent.oldPos.y);
		}
	}

	//Hitting a deadly meteor
	for (int id : vMeteors)
	{
		GameObject& obj_meteor = Play::GetGameObject(id);
		if (Play::IsColliding(obj_agent, obj_meteor))
		{
			Play::PlayAudio("combust");
			gameState.agentStates = STATE_DEAD;
		}
	}

	//Collecting gems
	for (int id : vGems)
	{
		GameObject& obj_gem = Play::GetGameObject(id);
		if (Play::IsColliding(obj_agent, obj_gem))
		{
			gameState.score++;
			Play::PlayAudio("collect");
			int id_ring = Play::CreateGameObject(TYPE_RING, obj_gem.pos, 0, "blue_ring");
			GameObject& obj_ring = Play::GetGameObject(id_ring);
			obj_ring.scale = 0.25;
			Play::DestroyGameObject(id);
		}
	}
}

void StateAttached()
{
	//Variables
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_attached = Play::GetGameObjectByType(TYPE_ATTACHED);
	int id_attached = obj_attached.GetId();

	AgentAttached();

	//Left and right movement
	if (Play::KeyDown(VK_RIGHT))
	{
		obj_agent.rotSpeed = 0.05;
		Play::SetSprite(obj_agent, "agent8_right_7", 0.5);
	}
	else if (Play::KeyDown(VK_LEFT))
	{
		obj_agent.rotSpeed = -0.05;
		Play::SetSprite(obj_agent, "agent8_left_7", 0.5);
	}
	else
	{
		obj_agent.rotSpeed = 0;
		obj_agent.animSpeed = 0;
	}

	//Launch from asteroid
	if (Play::KeyPressed(VK_SPACE))
	{
		gameState.agentStates = STATE_FLYING;
		Play::PlayAudio("explode");
		obj_agent.pos.x = obj_agent.pos.x + 20 * sin(obj_agent.rotation);
		obj_agent.pos.y = obj_agent.pos.y + 20 * -cos(obj_agent.rotation);
		obj_agent.animSpeed = 0.1;
		SpawnPieces(obj_attached);
		SpawnGems(id_attached);
		Play::DestroyGameObject(id_attached);
	}
}

void StateDead()
{
	//Agent
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	Play::SetSprite(obj_agent, "agent8_dead", 1);
	obj_agent.rotSpeed = 0;
	Play::SetGameObjectDirection(obj_agent, 15, obj_agent.rotation);
	if (Play::IsLeavingDisplayArea(obj_agent))
	{
		WrapMovement(obj_agent);
	}

	//Restart current level
	if (Play::KeyPressed(VK_SPACE))
	{
		Restart(gameState.startingLevel);
		Play::SetSprite(obj_agent, "agent8_left_7", 0);
	}
}

void StateStart()
{
	//Attach to asteroid and reset rotation
	AgentAttached();
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	obj_agent.rotSpeed = 0;

	//Instructions
	Play::DrawFontText("151px", "Level " + std::to_string(gameState.startingLevel - 1), { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
	Play::DrawFontText("64px", "Collect " + std::to_string(gameState.gemNumber) + " gem(s)", { DISPLAY_WIDTH / 2 - 17, DISPLAY_HEIGHT / 2 + 100 }, Play::CENTRE);
	Play::DrawFontText("64px", "Left and right keys to move, spacebar to jump", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 50 }, Play::CENTRE);
	
	//Start Game
	if (Play::KeyPressed(VK_SPACE))
	{
		gameState.agentStates = STATE_ATTACHED;
	}
}

void AgentAttached()
{
	//Variables
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	GameObject& obj_attached = Play::GetGameObjectByType(TYPE_ATTACHED);
	int id_attached = obj_attached.GetId();

	//Attach to asteroid
	obj_agent.pos = obj_attached.pos;
	Play::SetGameObjectDirection(obj_attached, 4, obj_attached.rotation);
	obj_agent.velocity = obj_attached.velocity;

	//Update attached asteroid
	Play::UpdateGameObject(obj_attached);
	Play::DrawObjectRotated(obj_attached);
	if (Play::IsLeavingDisplayArea(obj_attached))
	{
		WrapMovement(obj_attached);
	}
}

void SpawnPieces(GameObject& object)
{
	float rad = 0.25f;
	//Three pieces in three opposite directions
	for (int i = 1; i <= 3; i++)
	{
		int id_piece = Play::CreateGameObject(TYPE_PIECES, object.pos, 0, "asteroid_pieces");
		GameObject& obj_piece = Play::GetGameObject(id_piece);
		Play::SetGameObjectDirection(obj_piece, 10, rad);
		obj_piece.frame = i;
		rad += 0.5f;
	}
}

void UpdatePieces()
{
	//Movement of pieces
	std::vector<int> vPieces = Play::CollectGameObjectIDsByType(TYPE_PIECES);
	for (int id : vPieces)
	{
		GameObject& obj_piece = Play::GetGameObject(id);
		Play::DrawObjectRotated(obj_piece);
		Play::UpdateGameObject(obj_piece);
		if (!Play::IsVisible(obj_piece))
		{
			Play::DestroyGameObject(id);
		}
	}
}

void SpawnGems(int id_rock)
{
	GameObject& obj_rock = Play::GetGameObject(id_rock);
	//Only if total number of gems for level isn't yet reached
	if (gameState.gemsSpawned < gameState.gemNumber)
	{
		//Only for odd number asteroids
		if (id_rock % 2)
		{
			int id_gem = Play::CreateGameObject(TYPE_WAITING, obj_rock.pos, 20, "gem");
			GameObject& obj_gem = Play::GetGameObject(id_gem);

			//Check in display area
			if (obj_gem.pos.y >= DISPLAY_HEIGHT)
			{
				obj_gem.pos.y = DISPLAY_HEIGHT - 20;
			}
			if (obj_gem.pos.x >= DISPLAY_WIDTH)
			{
				obj_gem.pos.x = DISPLAY_WIDTH - 20;
			}
			if (obj_gem.pos.y <= 0)
			{
				obj_gem.pos.y = 20;
			}
			if (obj_gem.pos.x <= 0)
			{
				obj_gem.pos.x = 20;
			}
			obj_gem.animSpeed = 0.5;
			gameState.gemsSpawned++;
		}
	}	
}

void UpdateGems()
{
	//Update gems-in-waiting
	std::vector<int> vWait = Play::CollectGameObjectIDsByType(TYPE_WAITING);
	for (int id : vWait)
	{
		GameObject& obj_waiting = Play::GetGameObject(id);
		Play::UpdateGameObject(obj_waiting);
		int waitTime = 10;
		//Delay before allowing agent collision
		if (obj_waiting.frame == 10)
		{
			obj_waiting.type = TYPE_GEM;
			obj_waiting.animSpeed = 0;
			obj_waiting.rotSpeed = 0.05;
		}
	}


	//Update gems
	std::vector<int> vGems = Play::CollectGameObjectIDsByType(TYPE_GEM);
	for (int id : vGems)
	{
		GameObject& obj_gem = Play::GetGameObject(id);
		Play::DrawObjectRotated(obj_gem);
		Play::UpdateGameObject(obj_gem);

		//Animation
		if (obj_gem.rotation >= 1 || obj_gem.rotation <= -1)
		{
			obj_gem.rotSpeed *= -1;
		}
	}
}

void WrapMovement(GameObject& object) //Wrap around display area when out of sight
{
	if (object.pos.x > DISPLAY_WIDTH + 100)
	{
		object.pos.x -= object.oldPos.x;
	}
	if (object.pos.y > DISPLAY_HEIGHT + 100)
	{
		object.pos.y -= object.oldPos.y;
	}
	if (object.pos.x < -100)
	{
		object.pos.x += object.oldPos.x * -1 + DISPLAY_WIDTH;
	}
	if (object.pos.y < -100)
	{
		object.pos.y += object.oldPos.y * -1 + DISPLAY_HEIGHT;
	}
}

void UpdateRings()
{
	std::vector<int> vRings = Play::CollectGameObjectIDsByType(TYPE_RING);
	for (int id : vRings)
	{
		GameObject& obj_ring = Play::GetGameObject(id);
		obj_ring.scale += 0.1;
		Play::UpdateGameObject(obj_ring);
		Play::DrawObjectRotated(obj_ring);
		if (obj_ring.scale >= 1.5)
		{
			Play::DestroyGameObject(id);
		}
	}
}

void SpawnParticles()
{
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);
	int posOffset_x = Play::RandomRoll(5);
	int posOffset_y = Play::RandomRoll(5);
	int id_particles = Play::CreateGameObject(TYPE_PARTICLES, { obj_agent.oldPos.x + posOffset_x, obj_agent.oldPos.y + posOffset_y }, 0, "particle");
}

void UpdateParticles()
{
	std::vector<int> vParticles = Play::CollectGameObjectIDsByType(TYPE_PARTICLES);
	for (int id : vParticles)
	{
		GameObject& obj_particles = Play::GetGameObject(id);
		obj_particles.scale -= 0.02;
		Play::UpdateGameObject(obj_particles);
		Play::DrawObjectRotated(obj_particles, obj_particles.scale);
		if (obj_particles.scale <= 0.05)
		{
			Play::DestroyGameObject(id);
		}
	}
}

void Restart(int level)
{
	GameObject& obj_agent = Play::GetGameObjectByType(TYPE_AGENT8);

	//Destroy objects
	std::vector<std::vector<int>> vDestroy =
	{
		{ Play::CollectGameObjectIDsByType(TYPE_ASTEROID) },
		{ Play::CollectGameObjectIDsByType(TYPE_METEOR) },
		{ Play::CollectGameObjectIDsByType(TYPE_GEM) },
		{ Play::CollectGameObjectIDsByType(TYPE_PIECES) },
	};
	for (std::vector<int> v : vDestroy)
	{
		for (int id : v)
		{
			Play::DestroyGameObject(id);
		}
	}

	//Spawn new asteroids an meteors
	SpawnRocks(level);
	SpawnMeteors(level);

	//Reset variables
	gameState.score = 0;
	gameState.gemsSpawned = 0;
	gameState.agentStates = STATE_START;
	Play::SetSprite(obj_agent, "agent8_left", 0);
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

