#pragma once

#include <Box2D/Box2D.h>

#include <Kengine/IGameScreen.h>
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLSLProgram.h>
#include <Kengine/Camera2D.h>
#include <Kengine/GLTexture.h>
#include <Kengine/Window.h>


#include <vector>

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
	Kengine::Camera2D m_camera;
	Kengine::GLTexture m_texture;
	Kengine::Window* m_window;

	std::vector<Box> m_boxes;
	std::unique_ptr<b2World> m_world;
};

