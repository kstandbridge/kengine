#pragma once

#include "Capsule.h"
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLTexture.h>
#include <Kengine/InputManager.h>


class Player
{
public:

	void init(b2World* world, 
			  const glm::vec2 position, 
			  const glm::vec2 drawDims, 
			  const glm::vec2 collisionDims, 
			  Kengine::ColorRGBA8 color);

	void draw(Kengine::SpriteBatch& spriteBatch);
	void drawDebug(Kengine::DebugRenderer& debugRenderer);

	void update(Kengine::InputManager& inputManager);

	const Capsule& getBox() const { return m_capsule; }

private:
	glm::vec2 m_drawDims;
	Kengine::ColorRGBA8 m_color;
	Kengine::GLTexture m_texture;
	Capsule m_capsule;
};
