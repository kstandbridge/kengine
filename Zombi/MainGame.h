#pragma once

#include <Kengine/Window.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/Camera2D.h>
#include <Kengine/InputManager.h>

#include "Player.h"
#include "Level.h"
#include "Bullet.h"

class Zombie;

enum class GameState { PLAY, EXIT };

class MainGame
{
public:
    MainGame();
    ~MainGame();

    /// Runs the game
    void run();

private:
    /// Initializes the core systems
    void initSystems();

	/// Initalizes the level and sets up everything
	void initLevel();

    /// Initializes the shaders
    void initShaders();

    /// Main game loop for the program
    void gameLoop();

	/// Updates all agents
	void updateAgents(float deltaTime);

	/// Updates all bullet
	void updateBullets(float deltaTime);

	/// Checks the victory condition
	void checkVictory();

    /// Handles input processing
    void processInput();
    
	/// Renders the game
    void drawGame();

private:
    /// Member Variables
    Kengine::Window _window; ///< The game window
	Kengine::GLSLProgram _textureProgram; ///< The shader program
	Kengine::InputManager _inputManager; ///< Handles input
    Kengine::Camera2D _camera; ///< Main Camera

	Kengine::SpriteBatch _agentSpriteBatch;

	std::vector<Level*> _levels;

	int _screenWidth, _screenHeight;

	float _fps;

	int _currentLevel;

	Player* _player;
	std::vector<Human*> _humans;
	std::vector<Zombie*> _zombies;
	std::vector<Bullet> _bullets;

	int _numHumansKilled; /// Humans killed by player
	int _numZombiesKilled; /// Zombies killed by player

	GameState _gameState;
};

