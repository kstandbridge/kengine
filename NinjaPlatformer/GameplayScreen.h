#pragma once

#include <Box2D/Box2D.h>
#include <Kengine/IGameScreen.h>
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/Camera2D.h>
#include <Kengine/GLTexture.h>
#include <Kengine/Window.h>
#include <Kengine/DebugRenderer.h>
#include <Kengine/GUI.h>

#include <vector>

#include "Player.h"
#include "Box.h"

class GameplayScreen : public Kengine::IGameScreen
{
public:
	GameplayScreen(Kengine::Window* window);
	~GameplayScreen();

	int getNextScreenIndex() const override;

	int getPreviousScreenIndex() const override;

	void build() override;
	
	void destory() override;
	
	void onEntry() override;
	
	void onExit() override;
	
	void update() override;
	
	void draw() override;

private:
	void checkInput();

	Kengine::SpriteBatch m_spriteBatch;
	Kengine::GLSLProgram m_textureProgram;
	Kengine::GLSLProgram m_lightProgram;
	Kengine::Camera2D m_camera;
	Kengine::GLTexture m_texture;
	Kengine::Window* m_window;
	Kengine::DebugRenderer m_debugRenderer;
	Kengine::GUI m_gui;

	bool m_renderDebug = false;

	Player m_player;
	std::vector<Box> m_boxes;
	std::unique_ptr<b2World> m_world;
};

