#include "MainGame.h"

#include <Kengine/Kengine.h>
#include <Kengine/Timing.h>
#include <Kengine/KengineErrors.h>
#include <Kengine/ResourceManager.h>

#include <random>
#include <ctime>
#include <algorithm>

#include <SDL/SDL.h>
#include <glm/gtx/rotate_vector.hpp>

#include "Gun.h";
#include "Zombie.h";
#include <iostream>


const float HUMAN_SPEED = 1.0f;
const float ZOMBIE_SPEED = 1.3f;
const float PLAYER_SPEED = 8.0f;

MainGame::MainGame()
{
	// Empty
}

MainGame::~MainGame() 
{
	for(int i = 0; i < m_levels.size(); i++)
	{
		delete m_levels[i];
	}

	for(int i = 0; i < m_humans.size(); i++)
	{
		delete m_humans[i];
	}

	for(int i = 0; i < m_zombies.size(); i++)
	{
		delete m_zombies[i];
	}
}

void MainGame::run() 
{
	initSystems();
	initLevel();

	Kengine::Music music = m_audioEngine.loadMusic("Sound/XYZ.ogg");
	music.play(-1);
	
	gameLoop();
}

void MainGame::initSystems()
{
	Kengine::init();

	m_audioEngine.init();

	m_window.create("Zombi", m_screenWidth, m_screenHeight, 0);
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

	initShaders();

	m_agentSpriteBatch.init();
	m_hudSpriteBatch.init();

	m_spriteFont = new Kengine::SpriteFont("Fonts/chintzy.ttf", 64);

	m_camera.init(m_screenWidth, m_screenHeight);
	
	m_hudCamera.init(m_screenWidth, m_screenHeight);
	glm::vec2 offSetCameraPosition = glm::vec2(m_screenWidth / 2, m_screenHeight / 2);
	m_hudCamera.setPosition(offSetCameraPosition);

	m_bloodParticleBatch = new Kengine::ParticleBatch2D();
	m_bloodParticleBatch->init(1000, 
							   0.05f,
							   Kengine::ResourceManager::getTexture("Textures/particle.png"),
							   [](Kengine::Particle2D& particle, float deltaTime)
							   {
							   		particle.position += particle.velocity * deltaTime;
									particle.color.a = (GLubyte)(particle.life * 255.0f);
							   });
	m_particleEngine.addParticleBatch(m_bloodParticleBatch);
}

void MainGame::initLevel()
{
	// Level 1
	m_levels.push_back(new Level("Levels/LevelSmall.txt"));
//	m_levels.push_back(new Level("Levels/LevelBig.txt"));
	m_currentLevel = 0;

	m_player = new Player();
	m_player->init(PLAYER_SPEED, m_levels[m_currentLevel]->getStartPlayerPos(), &m_inputManager, &m_camera, &m_bullets);

	m_humans.push_back(m_player);

	std::mt19937 randomEngine;
	randomEngine.seed(time(nullptr));
	std::uniform_int_distribution<int> randX(2, m_levels[m_currentLevel]->getWidth() - 2);
	std::uniform_int_distribution<int> randY(2, m_levels[m_currentLevel]->getHeight() - 2);

	// Add all the random humans
	for(int i = 0; i < m_levels[m_currentLevel]->getNumHumans(); i++)
	{
		m_humans.push_back(new Human);
		glm::vec2 pos(randX(randomEngine) * TILE_WIDTH, randY(randomEngine) * TILE_WIDTH);
		m_humans.back()->init(HUMAN_SPEED, pos);
	}

	// Add the zombies
	const std::vector<glm::ivec2> zombiePositions = m_levels[m_currentLevel]->getZombiStartPositions();
	for(int i = 0; i < zombiePositions.size(); i++)
	{
		m_zombies.push_back(new Zombie);
		m_zombies.back()->init(ZOMBIE_SPEED, zombiePositions[i]);
	}

	// Set up the player guns
	const float BULLET_SPEED = 20.0f;
	m_player->addGun(new Gun("Magnum", 10, 1, 5.0f, 30, BULLET_SPEED, m_audioEngine.loadSoundEffect("Sound/shots/pistol.wav")));
	m_player->addGun(new Gun("Shotgun", 30, 12, 20.0f, 4, BULLET_SPEED, m_audioEngine.loadSoundEffect("Sound/shots/shotgun.wav")));
	m_player->addGun(new Gun("MP5", 2, 1, 10.0f, 20, BULLET_SPEED, m_audioEngine.loadSoundEffect("Sound/shots/cg1.wav")));
}

void MainGame::initShaders()
{
    // Compile our color shader
	m_textureProgram.compileShaders("Resources/Shaders/vertexShader.vs", "Resources/Shaders/fragmentShader.fs");
	m_textureProgram.addAttribute("vertexPosition");
	m_textureProgram.addAttribute("vertexColor");
	m_textureProgram.addAttribute("vertexUV");
	m_textureProgram.linkShaders();
}

void MainGame::gameLoop()
{
	const float DESIRED_FPS = 60.0f;
	const int MAX_PHYSICIS_STEPS = 6;

	Kengine::FpsLimiter fpsLimiter;
	fpsLimiter.setMaxFPS(1200.0f);

	const float CAMERA_SCALE = 1.0f / 2.0f;
	m_camera.setScale(CAMERA_SCALE);

	const float MS_PER_SECOND = 1000;
	const float DESIRED_FRAMETIME = MS_PER_SECOND / DESIRED_FPS;
	const float MAX_DELTA_TIME = 1.0f;

	float previousTicks = SDL_GetTicks();

	while(m_gameState == GameState::PLAY)
	{
		fpsLimiter.begin();

		float newTicks = SDL_GetTicks();
		float frameTime = newTicks - previousTicks;
		previousTicks = newTicks;
		float totalDeltaTime = frameTime / DESIRED_FRAMETIME;

		checkVictory();

		m_inputManager.update();
		
		processInput();

		int i = 0;
		while(totalDeltaTime > 0.0f && i < MAX_PHYSICIS_STEPS)
		{
			float deltaTime = std::min(totalDeltaTime, MAX_DELTA_TIME);
			updateAgents(deltaTime);
			updateBullets(deltaTime);
			m_particleEngine.update(deltaTime);
			totalDeltaTime -= deltaTime;
			i++;
		}

		

		m_camera.setPosition(m_player->getPosition());
		m_camera.update();

		m_hudCamera.update();

		drawGame();

		m_fps = fpsLimiter.end();
		std::cout << m_fps << std::endl;
	}
}

void MainGame::updateAgents(float deltaTime)
{
	// Update all humans
	for(int i = 0; i < m_humans.size(); i++)
	{
		m_humans[i]->update(m_levels[m_currentLevel]->getLevelData(),
						   m_humans,
						   m_zombies,
						   deltaTime);
	}

	// Update zombies
		for(int i = 0; i < m_zombies.size(); i++)
	{
		m_zombies[i]->update(m_levels[m_currentLevel]->getLevelData(),
						   m_humans,
						   m_zombies,
						   deltaTime);
	}

	// Update Zombie collisions
	for(int i = 0; i < m_zombies.size(); i++)
	{
		// Collide with other zombies
		for(int j = i + 1 ; j < m_zombies.size(); j++)
		{
			m_zombies[i]->collideWithAgent(m_zombies[j]);
		}

		// Collide with humans
		for(int j = 1 ; j < m_humans.size(); j++)
		{
			if(m_zombies[i]->collideWithAgent(m_humans[j]))
			{
				// Add the new zombie
				m_zombies.push_back(new Zombie);
				m_zombies.back()->init(ZOMBIE_SPEED, m_humans[j]->getPosition());

				// Delete the human
				delete m_humans[j];
				m_humans[j] = m_humans.back();
				m_humans.pop_back();
			}
		}

		// Collide with player
		if(m_zombies[i]->collideWithAgent(m_player))
		{
			Kengine::fatalError("YOU LOSE");
		}
	}

	// Update Human collisions
	for(int i = 0; i < m_humans.size(); i++)
	{
		// Collide with other humans
		for(int j = i + 1 ; j < m_humans.size(); j++)
		{
			m_humans[i]->collideWithAgent(m_humans[j]);
		}
	}

}

void MainGame::updateBullets(float deltaTime)
{
	// Update and collide with world
	for(int i = 0; i < m_bullets.size();)
	{
		// If update returns true, the bullet collided with a wall
		if(m_bullets[i].update(m_levels[m_currentLevel]->getLevelData(), deltaTime))
		{
			m_bullets[i] = m_bullets.back();
			m_bullets.pop_back();
		}
		else
		{
			i++;
		}
	}

	bool wasBulletRemoved;

	// Collide with humans and zombies
	for(int i = 0; i < m_bullets.size(); i++)
	{
		wasBulletRemoved = false;
		// Loop through zombies
		for(int j = 0; j < m_zombies.size();)
		{
			// Check collision
			if(m_bullets[i].collideWithAgent(m_zombies[j]))
			{
				// Add blood
				addBlood(m_bullets[i].getPosition(), 5);
				// Damage zombie, and kill it if its out of health
				if(m_zombies[j]->applyDamage(m_bullets[i].getDamage()))
				{
					// If the zombie died, remove him
					delete m_zombies[j];
					m_zombies[j] = m_zombies.back();
					m_zombies.pop_back();
					m_numZombiesKilled++;
				}
				else
				{
					j++;
				}

				// Remove the bullet
				m_bullets[i] = m_bullets.back();
				m_bullets.pop_back();
				wasBulletRemoved = true;
				// Since the bullet hit, no need to loop through any more zombies
				break;
			}
			else
			{
				j++;
			}
		}

		// Loop through humans
		if(wasBulletRemoved == false)
		{
			for(int j = 1; j < m_humans.size();)
			{
				// Check collision
				if(m_bullets[i].collideWithAgent(m_humans[j]))
				{
					// Add blood
					addBlood(m_bullets[i].getPosition(), 5);

					// Damage zombie, and kill it if its out of health
					if(m_humans[j]->applyDamage(m_bullets[i].getDamage()))
					{
						// If the zombie died, remove him
						delete m_humans[j];
						m_humans[j] = m_humans.back();
						m_humans.pop_back();
						m_numHumansKilled++;
					}
					else
					{
						j++;
					}

					// Remove the bullet
					m_bullets[i] = m_bullets.back();
					m_bullets.pop_back();
					// Since the bullet hit, no need to loop through any more zombies
					break;
				}
				else
				{
					j++;
				}
			}
		}
	}
}

void MainGame::checkVictory()
{
	// TODO: Support for multiple levels!
	// _currentlLevel++; initLevel(...)

	// If all zomies are dead we win!
	if(m_zombies.empty())
	{
		std::printf("*** You win ***\nYou killed %d civilians and %d zombies. There are %d/%d civilians remaining",
					m_numHumansKilled, m_numZombiesKilled, m_humans.size() - 1, m_levels[m_currentLevel]->getNumHumans());
		
		Kengine::fatalError("");
	}
}

void MainGame::drawHud()
{
	char buffer[256];

	glm::mat4 projectionMatrix = m_hudCamera.getCameraMatrix();
	GLint pUniform = m_textureProgram.getUniformLocation("transformationMatrix");
	glUniformMatrix4fv(pUniform, 1, GL_FALSE, &projectionMatrix[0][0]);

	m_hudSpriteBatch.begin();

	sprintf_s(buffer, "Num Humans %d", m_humans.size());

	m_spriteFont->draw(m_hudSpriteBatch, 
					  buffer, 
					  glm::vec2(0, 0), 
					  glm::vec2(0.5f), 
					  0.0f, 
					  Kengine::ColorRGBA8(255, 255, 255, 255),
					  Kengine::Justification::LEFT);

	sprintf_s(buffer, "Num Zombies %d", m_zombies.size());

	m_spriteFont->draw(m_hudSpriteBatch, 
					  buffer, 
					  glm::vec2(0, 36), 
					  glm::vec2(0.5f), 
					  0.0f, 
					  Kengine::ColorRGBA8(255, 255, 255, 255),
					  Kengine::Justification::LEFT);
	
	m_hudSpriteBatch.end();
	m_hudSpriteBatch.renderBatch();
}

void MainGame::addBlood(const glm::vec2& position, int numParticles)
{
	static std::mt19937 randEngine(time(nullptr));
	static std::uniform_real_distribution<float> randAngle(0.0f, 360.0f);

	glm::vec2 vel(2.0f, 0.0f);
	Kengine::ColorRGBA8 color(255, 0, 0, 255);

	for (int i = 0; i < numParticles; i++)
	{
		m_bloodParticleBatch->addParticle(position, 
										  glm::rotate(vel, randAngle(randEngine)), 
										  color, 
										  20.0f);
	}
}

void MainGame::processInput() {
    SDL_Event evnt;

    //Will keep looping until there are no more events to process
    while (SDL_PollEvent(&evnt)) {
        switch (evnt.type) {
            case SDL_QUIT:
                m_gameState = GameState::EXIT;
                break;
            case SDL_MOUSEMOTION:
                m_inputManager.setMouseCoords(evnt.motion.x, evnt.motion.y);
                break;
            case SDL_KEYDOWN:
				m_inputManager.pressKey(evnt.key.keysym.sym);
                break;
            case SDL_KEYUP:
				m_inputManager.releaseKey(evnt.key.keysym.sym);
                break;
            case SDL_MOUSEBUTTONDOWN:
				m_inputManager.pressKey(evnt.button.button);
                break;
            case SDL_MOUSEBUTTONUP:
				m_inputManager.releaseKey(evnt.button.button);
                break;
        }
    
	}

	

//	if(_inputManager.isKeyDown(SDLK_q))
//	{
//		_camera.setScale(_camera.getScale() * (1 + SCALE_SPEED));
//	}
//	
//	if(_inputManager.isKeyDown(SDLK_e))
//	{
//		_camera.setScale(_camera.getScale() * (1 - SCALE_SPEED));
//	}

}

void MainGame::drawGame() {
    // Set the base depth to 1.0
    glClearDepth(1.0);
    // Clear the color and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Bind the shader
	m_textureProgram.use();

	// Draw code goes here
	glActiveTexture(GL_TEXTURE0);

	// Make sure the shader uses texture 0
	GLint textureUniform = m_textureProgram.getUniformLocation("mySampler");
	glUniform1i(textureUniform, 0);

	// Grab the camera matrix
	glm::mat4 projectionMatrix = m_camera.getCameraMatrix();
	GLint pUniform = m_textureProgram.getUniformLocation("transformationMatrix");
	glUniformMatrix4fv(pUniform, 1, GL_FALSE, &projectionMatrix[0][0]);

	// Draw the level
	m_levels[m_currentLevel]->draw();

	m_agentSpriteBatch.begin();

	const glm::vec2 agentDims(AGENT_RADIUS * 2.0f);

	// Draw the humans
	for (auto i = 0; i < m_humans.size(); i++)
	{
		if(m_camera.isBoxInView(m_humans[i]->getPosition(), agentDims))
		{
			m_humans[i]->draw(m_agentSpriteBatch);
		}
	}

	// Draw the zombies
	for (auto i = 0; i < m_zombies.size(); i++)
	{
		if(m_camera.isBoxInView(m_zombies[i]->getPosition(), agentDims))
		{
			m_zombies[i]->draw(m_agentSpriteBatch);
		}
	}

	// Draw the bullets
	for(int i = 0; i < m_bullets.size(); i++)
	{
		m_bullets[i].draw(m_agentSpriteBatch);
	}

	m_agentSpriteBatch.end();

	m_agentSpriteBatch.renderBatch();

	m_particleEngine.draw(&m_agentSpriteBatch);

	drawHud();

	// Unbind the shader
	m_textureProgram.unuse();

	//Swap our buffer and draw everything to the screen!
	m_window.swapBuffer();
}