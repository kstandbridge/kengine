#include "MainGame.h"

#include <Kengine/KengineErrors.h>
#include "Kengine/ResourceManager.h"

#include <iostream>
#include <string>

MainGame::MainGame()
	: m_screenWidth{1024},
	  m_screenHeight{768},
	  m_gameState{GameState::PLAY},
	  _maxFPS(60.0f), 
	  m_fps{0},
	  _time{0}
{
	m_camera.init(m_screenWidth, m_screenHeight);
}

MainGame::~MainGame()
{	
}

void MainGame::run()
{
	initSystems();

	gameLoop();
}

void MainGame::initSystems()
{
	Kengine::init();

	m_window.create("Game Engine", m_screenWidth, m_screenHeight, 0);

	initShaders();

	_spriteBatch.init();
	_fpsLimiter.init(_maxFPS);
}

void MainGame::initShaders()
{
	_colorProgram.compileShaders("Shaders/colorShading.vert", "Shaders/colorShading.frag");
	_colorProgram.addAttribute("vertexPosition");
	_colorProgram.addAttribute("vertexColor");
	_colorProgram.addAttribute("vertexUV");
	_colorProgram.linkShaders();
}

void MainGame::gameLoop()
{
	while(m_gameState != GameState::EXIT)
	{
		_fpsLimiter.begin();

		processInput();
		_time += 0.05f;

		m_camera.update();

		for(int i = 0; i < m_bullets.size();)
		{
			if(m_bullets[i].update())
			{
				m_bullets[i] = m_bullets.back();
				m_bullets.pop_back();
			}
			else
			{
				i++;
			}
		}

		drawGame();

		m_fps = _fpsLimiter.end();

		// Print only once every 10 frames
		static int frameCounter = 0;
		frameCounter++;
		if(frameCounter == 10000)
		{
			std::cout << m_fps << std::endl;
			frameCounter = 0;
		}
	}
}

void MainGame::processInput()
{
	SDL_Event evnt;

	const static float CAMERA_SPEED = 2.0f;
	const static float SCALE_SPEED = 0.1f;

	while(SDL_PollEvent(&evnt))
	{
		switch(evnt.type)
		{
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

	if(m_inputManager.isKeyDown(SDLK_w))
	{
		m_camera.setPosition(m_camera.getPosition() + glm::vec2(0.0f, CAMERA_SPEED));
	}
	
	if(m_inputManager.isKeyDown(SDLK_s))
	{
		m_camera.setPosition(m_camera.getPosition() + glm::vec2(0.0f, -CAMERA_SPEED));
	}
	
	if(m_inputManager.isKeyDown(SDLK_a))
	{
		m_camera.setPosition(m_camera.getPosition() + glm::vec2(-CAMERA_SPEED, 0.0f));
	}
	
	if(m_inputManager.isKeyDown(SDLK_d))
	{
		m_camera.setPosition(m_camera.getPosition() + glm::vec2(CAMERA_SPEED, 0.0f));
	}
	
	if(m_inputManager.isKeyDown(SDLK_q))
	{
		m_camera.setScale(m_camera.getScale() * (1 + SCALE_SPEED));
	}
	
	if(m_inputManager.isKeyDown(SDLK_e))
	{
		m_camera.setScale(m_camera.getScale() * (1 - SCALE_SPEED));
	}

	if(m_inputManager.isKeyDown(SDL_BUTTON_LEFT))
	{
		glm::vec2 mouseCoords = m_inputManager.getMouseCoords();
		mouseCoords = m_camera.convertScreenToWorld(mouseCoords);
		
		std::cout << mouseCoords.x << ", " << mouseCoords.y << std::endl;

		glm::vec2 playerPosition(0.0f);
		glm::vec2 direction = mouseCoords - playerPosition;
		direction = glm::normalize(direction);

		m_bullets.emplace_back(Bullet(playerPosition, direction, 5.00f, 1000));
	}
}

void MainGame::drawGame()
{
	// Set the base depth to 1.0
	glClearDepth(1.0);

	// Clear the color and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//Enable the shader
	_colorProgram.use();

	// We are using texture unit 0
	glActiveTexture(GL_TEXTURE0);
	// GEt the uniform location
	GLint textureLocation = _colorProgram.getUniformLocation("mySampler");
	// Tell the shader that the texture is in texture unit 0
	glUniform1i(textureLocation, 0);

	// Set the camera matrix
	GLuint pLocation = _colorProgram.getUniformLocation("P");
	glm::mat4 cameraMatrix = m_camera.getCameraMatrix();

	// Upload to GPU?
	glUniformMatrix4fv(pLocation, 1, GL_FALSE, &(cameraMatrix[0][0]));


	_spriteBatch.begin(Kengine::GlyphSortType::TEXTURE);

	glm::vec4 pos(0.0f, 0.0f, 50.0f, 50.0f);
	glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);
	static Kengine::GLTexture texture = Kengine::ResourceManager::getTexture("Textures/jimmyJump_pack/PNG/CharacterRight_Standing.png");
	Kengine::ColorRGBA8 color;
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 255;

	_spriteBatch.draw(pos, uv, texture.id, 0.0f, color);

	for(int i = 0; i < m_bullets.size(); i++)
	{
		m_bullets[i].draw(_spriteBatch);
	}

	_spriteBatch.end();

	_spriteBatch.renderBatch();
		
	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// Disable the shader
	_colorProgram.unuse();

	m_window.swapBuffer();
}
