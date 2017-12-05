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
					    std::vector<Zombie*>& zombies) = 0;

	bool collideWithLevel(const std::vector<std::string>& levelData);

	bool collideWithAgent(Agent* agent);

	void draw(Kengine::SpriteBatch& spiteBatch);

	// Return true if we died
	bool applyDamage(float damage);

	glm::vec2 getPosition() const { return _position; }

protected:

	void checkTilePosition(const std::vector<std::string>& levelData,
						   std::vector<glm::vec2>& collideTilePositions,
						   float x,
						   float y);

	void collideWithTile(glm::vec2 tilePos);

	glm::vec2 _position;
	Kengine::ColorRGBA8 _color;
	float _speed;
	float _health;
};

