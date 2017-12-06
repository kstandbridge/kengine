#pragma once

#include <SDL/SDL.h>
#include <GL/glew.h>

#include <Kengine/Kengine.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/GLTexture.h>
#include <Kengine/Sprite.h>
#include <Kengine/Window.h>
#include <Kengine/InputManager.h>
#include <Kengine/Timing.h>

#include <Kengine/SpriteBatch.h>

#include <Kengine/Camera2D.h>

#include "Bullet.h"

#include <vector>

enum class GameState { PLAY, EXIT };

class MainGame
{
public:
	MainGame();
	~MainGame();

	void run();

private:
	void initSystems();
	void initShaders();
	void gameLoop();
	void processInput();
	void drawGame();

	Kengine::Window m_window;
	int m_screenWidth;
	int m_screenHeight;
	GameState m_gameState;

	Kengine::GLSLProgram _colorProgram;
	Kengine::Camera2D m_camera;

	Kengine::SpriteBatch _spriteBatch;

	Kengine::InputManager m_inputManager;
	Kengine::FpsLimiter _fpsLimiter;

	std::vector<Bullet> m_bullets;

	float _maxFPS;
	float m_fps;
	float _time;
};

