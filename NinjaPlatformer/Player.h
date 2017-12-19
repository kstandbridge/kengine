#pragma once

#include "Box.h"
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLTexture.h>
#include <Kengine/InputManager.h>

class Player
{
public:

	void init(b2World* world, const glm::vec2 position, const glm::vec2 dimensions, Kengine::ColorRGBA8 color);

	void draw(Kengine::SpriteBatch& spriteBatch);

	void update(Kengine::InputManager& inputManager);



	const Box& getBox() const { return m_collisionBox; }

private:

	Box m_collisionBox;
};
