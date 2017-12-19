#pragma once

#include "Capsule.h"
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLTexture.h>
#include <Kengine/InputManager.h>
#include <Kengine/TileSheet.h>

enum class PlayerMoveState { STANDING, RUNNING, PUNCHING, IN_AIR };

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
	Kengine::TileSheet m_texture;
	Capsule m_capsule;
	PlayerMoveState m_moveState = PlayerMoveState::STANDING;
	float m_animTime = 0.0f;
	int m_direction = 1; // 1 or -1
	bool m_onGround = false;
	bool m_isPunching = false;
};
