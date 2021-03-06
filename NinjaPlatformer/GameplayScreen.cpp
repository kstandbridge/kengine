#include <SDL/SDL.h>

#include <random>

#include <Kengine/IMainGame.h>
#include <Kengine/ResourceManager.h>

#include "GameplayScreen.h"
#include "Box.h"

#include "Light.h"

GameplayScreen::GameplayScreen(Kengine::Window* window)
	: m_window{window}
{
}

GameplayScreen::~GameplayScreen()
{
}

int GameplayScreen::getNextScreenIndex() const
{
	return SCREEN_INDEX_NO_SCREEN;
}

int GameplayScreen::getPreviousScreenIndex() const
{
	return SCREEN_INDEX_NO_SCREEN;
}

void GameplayScreen::build()
{
}

void GameplayScreen::destory()
{
}

void GameplayScreen::onEntry()
{

	b2Vec2 gravity{0.0f, -25.0f};
	m_world = std::make_unique<b2World>(gravity);

	m_debugRenderer.init();

	// Make the ground
	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.0f, -25.0f);
	b2Body* groundBody = m_world->CreateBody(&groundBodyDef);
	// Make the ground fixture
	b2PolygonShape groundBox;
	groundBox.SetAsBox(50.0f, 10.0f);
	groundBody->CreateFixture(&groundBox, 0.0f);

	// Load the texture
	m_texture = Kengine::ResourceManager::getTexture("Assets/bricks_top.png");

	// Make a bunch of boxes
	std::mt19937 randGenerator;
	std::uniform_real_distribution<float> xPos(-10.0f, 10.0f);
	std::uniform_real_distribution<float> yPos(-10.0f, 15.0f);
	std::uniform_real_distribution<float> size(0.5f, 2.5f);
	std::uniform_int_distribution<int> color(0, 255);
	int numBoxes = 10;
	for (int i = 0; i < numBoxes; i++)
	{
		Box newBox;
		Kengine::ColorRGBA8 randColor;
		randColor.r = color(randGenerator);
		randColor.g = color(randGenerator);
		randColor.b = color(randGenerator);
		randColor.a = 255;
		float dimension = size(randGenerator);
		newBox.init(m_world.get(),
		            glm::vec2(xPos(randGenerator), yPos(randGenerator)),
		            glm::vec2(dimension, dimension),
					m_texture,
					randColor,
					false);
		m_boxes.push_back(newBox);
	}

	// Initialize spritebatch
	m_spriteBatch.init();

	// Shader init
	// Compile our texture shader
	m_textureProgram.compileShaders("Shaders/vertexShader.vs", "Shaders/fragmentShader.fs");
	m_textureProgram.addAttribute("vertexPosition");
	m_textureProgram.addAttribute("vertexColor");
	m_textureProgram.addAttribute("vertexUV");
	m_textureProgram.linkShaders();

	// Compile our light shader
	m_lightProgram.compileShaders("Shaders/lightShading.vs", "Shaders/lightShading.fs");
	m_lightProgram.addAttribute("vertexPosition");
	m_lightProgram.addAttribute("vertexColor");
	m_lightProgram.addAttribute("vertexUV");
	m_lightProgram.linkShaders();

	// Init camera
	m_camera.init(m_window->getScreenWidth(), m_window->getScreenHeight());
	m_camera.setScale(24.0f);

	// Init player
	m_player.init(m_world.get(), 
				  glm::vec2(0.0f, 30.0f),
				  glm::vec2(2.0f), 
				  glm::vec2(1.0f, 1.8f), 
				  Kengine::ColorRGBA8(255, 255, 255, 255));

	// Init the GUI
	m_gui.init("GUI");
	m_gui.loadScheme("AlfiskoSkin.scheme");
	m_gui.loadScheme("TaharezLook.scheme");
	m_gui.setFont("DejaVuSans-10");
	
	CEGUI::PushButton* testButton = static_cast<CEGUI::PushButton*>(m_gui.createWidget("AlfiskoSkin/Button",
	                                                                                   glm::vec4(0.5f, 0.5f, 0.1f, 0.05f),
	                                                                                   glm::vec4(0.0f),
	                                                                                   "TestButton"));
	testButton->setText("Hello World!");
}

void GameplayScreen::onExit()
{
	m_debugRenderer.dispose();
}

void GameplayScreen::update()
{
	m_camera.update();
	checkInput();
	m_player.update(m_game->inputManager);

	// Update the physics simulation
	m_world->Step(1.0f / 60.0f, 6, 2);
}

void GameplayScreen::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	m_textureProgram.use();

	// Upload texture uniform
	GLuint textureUniform = m_textureProgram.getUniformLocation("mySampler");
	glUniform1i(textureUniform, 0);
	glActiveTexture(GL_TEXTURE0);

	// Camera matrix
	glm::mat4 projectionMatrix = m_camera.getCameraMatrix();
	GLint pUniform = m_textureProgram.getUniformLocation("P");
	glUniformMatrix4fv(pUniform, 1, GL_FALSE, &projectionMatrix[0][0]);

	m_spriteBatch.begin();

	// Draw all the boxes
	for (auto& b : m_boxes)
	{
		b.draw(m_spriteBatch);
	}

	m_player.draw(m_spriteBatch);

	m_spriteBatch.end();
	m_spriteBatch.renderBatch();
	m_textureProgram.unuse();

	// Debug rendering
	if(m_renderDebug)
	{
		glm::vec4 destRect;
		for (auto& b : m_boxes)
		{
			destRect.x = b.getBody()->GetPosition().x - b.getDimensions().x / 2.0f;
			destRect.y = b.getBody()->GetPosition().y - b.getDimensions().y / 2.0f;
			destRect.z = b.getDimensions().x;
			destRect.w = b.getDimensions().y;
			m_debugRenderer.drawBox(destRect, Kengine::ColorRGBA8(255, 255, 255, 255), b.getBody()->GetAngle());
		}
		
		// Render player
		m_player.drawDebug(m_debugRenderer);
		m_debugRenderer.end();
		m_debugRenderer.render(projectionMatrix, 2.0f);
	}

	// Render some test lights
	Light playerLight;
	playerLight.color = Kengine::ColorRGBA8(255, 255, 255, 128);
	playerLight.position = m_player.getPosition();
	playerLight.size = 30.0f;
	
	Light mouseLight;
	mouseLight.color = Kengine::ColorRGBA8(255, 0, 255, 150);
	mouseLight.position = m_camera.convertScreenToWorld(m_game->inputManager.getMouseCoords());
	mouseLight.size = 45.0f;
	
	m_lightProgram.use();
	pUniform = m_textureProgram.getUniformLocation("P");
	glUniformMatrix4fv(pUniform, 1, GL_FALSE, &projectionMatrix[0][0]);

	// Additive blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	
	m_spriteBatch.begin();

	playerLight.draw(m_spriteBatch);
	mouseLight.draw(m_spriteBatch);

	m_spriteBatch.end();
	m_spriteBatch.renderBatch();

	m_lightProgram.unuse();

	// Reset to regular alpha blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_gui.draw();
}

void GameplayScreen::checkInput()
{
	SDL_Event evnt;
	while (SDL_PollEvent(&evnt))
	{
		m_game->onSDLEvent(evnt);
	}
}
