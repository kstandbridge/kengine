#pragma once
#include <Kengine/Camera2D.h>
#include <Kengine/SpriteBatch.h>
#include <Kengine/InputManager.h>
#include <Kengine/Window.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/Timing.h>
#include <Kengine/SpriteFont.h>
#include <memory>

#include "BallController.h"
#include "BallRenderer.h"
#include "Grid.h"

enum class GameState { RUNNING, EXIT };

const int CELL_SIZE = 12;

class MainGame
{
public:
	void run();
	~MainGame();

private:
    void init();
	void initRenderers();
    void initBalls();
    void update(float deltaTime);
    void draw();
    void drawHud();
    void processInput();

    int m_screenWidth = 0;
    int m_screenHeight = 0;

    std::vector<Ball> m_balls; ///< All the balls
	std::unique_ptr<Grid> m_grid; ///< Grid for spatial partitioning for collision

    int m_currentRenderer = 0;
	std::vector<BallRenderer*> m_ballRenderers;

    BallController m_ballController; ///< Controls balls

    Kengine::Window m_window; ///< The main window
    Kengine::SpriteBatch m_spriteBatch; ///< Renders all the balls
    std::unique_ptr<Kengine::SpriteFont> m_spriteFont; ///< For font rendering
    Kengine::Camera2D m_camera; ///< Renders the scene
    Kengine::InputManager m_inputManager; ///< Handles input
    Kengine::GLSLProgram m_textureProgram; ///< Shader for textures]

    Kengine::FpsLimiter m_fpsLimiter; ///< Limits and calculates fps
    float m_fps = 0.0f;

    GameState m_gameState = GameState::RUNNING; ///< The state of the game
};
