#pragma once

#include <Kengine/Window.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/Camera2D.h>
#include <Kengine/InputManager.h>
#include <Kengine/SpriteBatch.h>
#include <Kengine/SpriteFont.h>

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

	/// Draws the HUD
	void drawHud();

private:
    /// Member Variables
    Kengine::Window m_window; ///< The game window
	Kengine::GLSLProgram m_textureProgram; ///< The shader program
	Kengine::InputManager m_inputManager; ///< Handles input
    
	Kengine::Camera2D m_camera; ///< Main Camera
    Kengine::Camera2D m_hudCamera; ///< HUD Camera

	Kengine::SpriteBatch m_agentSpriteBatch;
	Kengine::SpriteBatch m_hudSpriteBatch;

	std::vector<Level*> m_levels;

	int m_screenWidth = 1024;
	int m_screenHeight = 768;

	float m_fps = 0;

	int m_currentLevel;

	Player* m_player = nullptr;
	std::vector<Human*> m_humans;
	std::vector<Zombie*> m_zombies;
	std::vector<Bullet> m_bullets;

	int m_numHumansKilled = 0; /// Humans killed by player
	int m_numZombiesKilled = 0; /// Zombies killed by player

	Kengine::SpriteFont* m_spriteFont = nullptr;

	GameState m_gameState = GameState::PLAY;
};

