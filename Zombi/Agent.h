#pragma once

#include <glm/glm.hpp>
#include <Kengine/SpriteBatch.h>

const float AGENT_WIDTH = 32.0f;
const float AGENT_RADIUS = AGENT_WIDTH / 2;

class Zombie;
class Human;

class Agent
{
public:
	Agent();
	virtual ~Agent();

	virtual void update(const std::vector<std::string>& levelData,
					    std::vector<Human*>& humans,
					    std::vector<Zombie*>& zombies,
						float deltaTime) = 0;

	bool collideWithLevel(const std::vector<std::string>& levelData);

	bool collideWithAgent(Agent* agent);

	void draw(Kengine::SpriteBatch& spiteBatch);

	// Return true if we died
	bool applyDamage(float damage);

	glm::vec2 getPosition() const { return m_position; }

protected:

	void checkTilePosition(const std::vector<std::string>& levelData,
						   std::vector<glm::vec2>& collideTilePositions,
						   float x,
						   float y);

	void collideWithTile(glm::vec2 tilePos);

	glm::vec2 m_position;
	glm::vec2 m_direction = glm::vec2(1.0f, 0.0f);
	Kengine::ColorRGBA8 m_color;
	float m_speed;
	float m_health;
	GLuint m_textureId;
};

