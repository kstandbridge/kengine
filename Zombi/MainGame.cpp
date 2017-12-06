#include "MainGame.h"

#include <Kengine/Kengine.h>
#include <Kengine/Timing.h>
#include <Kengine/Errors.h>

#include <random>
#include <ctime>
#include <algorithm>

#include <SDL/SDL.h>

#include "Gun.h";
#include "Zombie.h";
#include <iostream>

const float HUMAN_SPEED = 1.0f;
const float ZOMBIE_SPEED = 1.3f;
const float PLAYER_SPEED = 8.0f;

MainGame::MainGame()
	: _screenWidth(1024),
	  _screenHeight(768),
	  _fps(0),
	  _player(nullptr),
	  _numHumansKilled(0),
	  _numZombiesKilled(0),
	  _gameState(GameState::PLAY)
{
	// Empty
}

MainGame::~MainGame() 
{
	for(int i = 0; i < _levels.size(); i++)
	{
		delete _levels[i];
	}

	for(int i = 0; i < _humans.size(); i++)
	{
		delete _humans[i];
	}

	for(int i = 0; i < _zombies.size(); i++)
	{
		delete _zombies[i];
	}
}

void MainGame::run() 
{
	initSystems();
	initLevel();

	gameLoop();


}

void MainGame::initSystems()
{
	Kengine::init();

	_window.create("Zombi", _screenWidth, _screenHeight, 0);
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

	initShaders();

	_agentSpriteBatch.init();

	_camera.init(_screenWidth, _screenHeight);
}

void MainGame::initLevel()
{
	// Level 1
	_levels.push_back(new Level("Levels/Level1.txt"));
	_currentLevel = 0;

	_player = new Player();
	_player->init(PLAYER_SPEED, _levels[_currentLevel]->getStartPlayerPos(), &_inputManager, &_camera, &_bullets);

	_humans.push_back(_player);

	std::mt19937 randomEngine;
	randomEngine.seed(time(nullptr));
	std::uniform_int_distribution<int> randX(2, _levels[_currentLevel]->getWidth() - 2);
	std::uniform_int_distribution<int> randY(2, _levels[_currentLevel]->getHeight() - 2);

	// Add all the random humans
	for(int i = 0; i < _levels[_currentLevel]->getNumHumans(); i++)
	{
		_humans.push_back(new Human);
		glm::vec2 pos(randX(randomEngine) * TILE_WIDTH, randY(randomEngine) * TILE_WIDTH);
		_humans.back()->init(HUMAN_SPEED, pos);
	}

	// Add the zombies
	const std::vector<glm::ivec2> zombiePositions = _levels[_currentLevel]->getZombiStartPositions();
	for(int i = 0; i < zombiePositions.size(); i++)
	{
		_zombies.push_back(new Zombie);
		_zombies.back()->init(ZOMBIE_SPEED, zombiePositions[i]);
	}

	// Set up the player guns
	const float BULLET_SPEED = 20.0f;
	_player->addGun(new Gun("Magnum", 10, 1, 5.0f, 30, BULLET_SPEED));
	_player->addGun(new Gun("Shotgun", 30, 12, 20.0f, 4, BULLET_SPEED));
	_player->addGun(new Gun("MP5", 2, 1, 10.0f, 20, BULLET_SPEED));
}

void MainGame::initShaders()
{
    // Compile our color shader
	_textureProgram.compileShaders("Resources/Shaders/vertexShader.vs", "Resources/Shaders/fragmentShader.fs");
	_textureProgram.addAttribute("vertexPosition");
	_textureProgram.addAttribute("vertexColor");
	_textureProgram.addAttribute("vertexUV");
	_textureProgram.linkShaders();
}

void MainGame::gameLoop()
{
	const float DESIRED_FPS = 60.0f;
	const int MAX_PHYSICIS_STEPS = 6;

	Kengine::FpsLimiter fpsLimiter;
	fpsLimiter.setMaxFPS(1200.0f);

	const float CAMERA_SCALE = 1.0f / 2.0f;
	_camera.setScale(CAMERA_SCALE);

	const float MS_PER_SECOND = 1000;
	const float DESIRED_FRAMETIME = MS_PER_SECOND / DESIRED_FPS;
	const float MAX_DELTA_TIME = 1.0f;

	float previousTicks = SDL_GetTicks();

	while(_gameState == GameState::PLAY)
	{
		fpsLimiter.begin();

		float newTicks = SDL_GetTicks();
		float frameTime = newTicks - previousTicks;
		previousTicks = newTicks;
		float totalDeltaTime = frameTime / DESIRED_FRAMETIME;

		checkVictory();

		_inputManager.update();
		
		processInput();

		int i = 0;
		while(totalDeltaTime > 0.0f && i < MAX_PHYSICIS_STEPS)
		{
			float deltaTime = std::min(totalDeltaTime, MAX_DELTA_TIME);
			updateAgents(deltaTime);
			updateBullets(deltaTime);
			totalDeltaTime -= deltaTime;
			i++;
		}

		_camera.setPosition(_player->getPosition());
		
		_camera.update();

		drawGame();

		_fps = fpsLimiter.end();
		std::cout << _fps << std::endl;
	}
}

void MainGame::updateAgents(float deltaTime)
{
	// Update all humans
	for(int i = 0; i < _humans.size(); i++)
	{
		_humans[i]->update(_levels[_currentLevel]->getLevelData(),
						   _humans,
						   _zombies,
						   deltaTime);
	}

	// Update zombies
		for(int i = 0; i < _zombies.size(); i++)
	{
		_zombies[i]->update(_levels[_currentLevel]->getLevelData(),
						   _humans,
						   _zombies,
						   deltaTime);
	}

	// Update Zombie collisions
	for(int i = 0; i < _zombies.size(); i++)
	{
		// Collide with other zombies
		for(int j = i + 1 ; j < _zombies.size(); j++)
		{
			_zombies[i]->collideWithAgent(_zombies[j]);
		}

		// Collide with humans
		for(int j = 1 ; j < _humans.size(); j++)
		{
			if(_zombies[i]->collideWithAgent(_humans[j]))
			{
				// Add the new zombie
				_zombies.push_back(new Zombie);
				_zombies.back()->init(ZOMBIE_SPEED, _humans[j]->getPosition());

				// Delete the human
				delete _humans[j];
				_humans[j] = _humans.back();
				_humans.pop_back();
			}
		}

		// Collide with player
		if(_zombies[i]->collideWithAgent(_player))
		{
			Kengine::fatalError("YOU LOSE");
		}
	}

	// Update Human collisions
	for(int i = 0; i < _humans.size(); i++)
	{
		// Collide with other humans
		for(int j = i + 1 ; j < _humans.size(); j++)
		{
			_humans[i]->collideWithAgent(_humans[j]);
		}
	}

}

void MainGame::updateBullets(float deltaTime)
{
	// Update and collide with world
	for(int i = 0; i < _bullets.size();)
	{
		// If update returns true, the bullet collided with a wall
		if(_bullets[i].update(_levels[_currentLevel]->getLevelData(), deltaTime))
		{
			_bullets[i] = _bullets.back();
			_bullets.pop_back();
		}
		else
		{
			i++;
		}
	}

	bool wasBulletRemoved;

	// Collide with humans and zombies
	for(int i = 0; i < _bullets.size(); i++)
	{
		wasBulletRemoved = false;
		// Loop through zombies
		for(int j = 0; j < _zombies.size();)
		{
			// Check collision
			if(_bullets[i].collideWithAgent(_zombies[j]))
			{
				// Damage zombie, and kill it if its out of health
				if(_zombies[j]->applyDamage(_bullets[i].getDamage()))
				{
					// If the zombie died, remove him
					delete _zombies[j];
					_zombies[j] = _zombies.back();
					_zombies.pop_back();
					_numZombiesKilled++;
				}
				else
				{
					j++;
				}

				// Remove the bullet
				_bullets[i] = _bullets.back();
				_bullets.pop_back();
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
			for(int j = 1; j < _humans.size();)
			{
				// Check collision
				if(_bullets[i].collideWithAgent(_humans[j]))
				{
					// Damage zombie, and kill it if its out of health
					if(_humans[j]->applyDamage(_bullets[i].getDamage()))
					{
						// If the zombie died, remove him
						delete _humans[j];
						_humans[j] = _humans.back();
						_humans.pop_back();
						_numHumansKilled++;
					}
					else
					{
						j++;
					}

					// Remove the bullet
					_bullets[i] = _bullets.back();
					_bullets.pop_back();
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
	if(_zombies.empty())
	{
		std::printf("*** You win ***\nYou killed %d civilians and %d zombies. There are %d/%d civilians remaining",
					_numHumansKilled, _numZombiesKilled, _humans.size() - 1, _levels[_currentLevel]->getNumHumans());
		
		Kengine::fatalError("");
	}
}

void MainGame::processInput() {
    SDL_Event evnt;

    //Will keep looping until there are no more events to process
    while (SDL_PollEvent(&evnt)) {
        switch (evnt.type) {
            case SDL_QUIT:
                _gameState = GameState::EXIT;
                break;
            case SDL_MOUSEMOTION:
                _inputManager.setMouseCoords(evnt.motion.x, evnt.motion.y);
                break;
            case SDL_KEYDOWN:
				_inputManager.pressKey(evnt.key.keysym.sym);
                break;
            case SDL_KEYUP:
				_inputManager.releaseKey(evnt.key.keysym.sym);
                break;
            case SDL_MOUSEBUTTONDOWN:
				_inputManager.pressKey(evnt.button.button);
                break;
            case SDL_MOUSEBUTTONUP:
				_inputManager.releaseKey(evnt.button.button);
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
	_textureProgram.use();

	// Draw code goes here
	glActiveTexture(GL_TEXTURE0);

	// Make sure the shader uses texture 0
	GLint textureUniform = _textureProgram.getUniformLocation("mySampler");
	glUniform1i(textureUniform, 0);

	// Grab the camera matrix
	glm::mat4 projectionMatrix = _camera.getCameraMatrix();
	GLint pUniform = _textureProgram.getUniformLocation("transformationMatrix");
	glUniformMatrix4fv(pUniform, 1, GL_FALSE, &projectionMatrix[0][0]);

	// Draw the level
	_levels[_currentLevel]->draw();

	_agentSpriteBatch.begin();

	// Draw the humans
	for (auto i = 0; i < _humans.size(); i++)
	{
		_humans[i]->draw(_agentSpriteBatch);
	}

	// Draw the zombies
	for (auto i = 0; i < _zombies.size(); i++)
	{
		_zombies[i]->draw(_agentSpriteBatch);
	}

	// Draw the bullets
	for(int i = 0; i < _bullets.size(); i++)
	{
		_bullets[i].draw(_agentSpriteBatch);
	}

	_agentSpriteBatch.end();

	_agentSpriteBatch.renderBatch();

	// Unbind the shader
	_textureProgram.unuse();

	//Swap our buffer and draw everything to the screen!
	_window.swapBuffer();
}