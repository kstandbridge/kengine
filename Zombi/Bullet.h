#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <Kengine/SpriteBatch.h>

class Human;
class Zombie;
class Agent;

const int BULLET_RADIUS = 5;

class Bullet
{
public:

	Bullet(const glm::vec2& position, 
		   const glm::vec2& direction,
		   const int damage, 
		   const float speed);
	~Bullet();

	// When update returns true, delete bullet
	bool update(const std::vector<std::string>& levelData, float deltaTime);

	void draw(Kengine::SpriteBatch& spriteBatch);

	bool collideWithAgent(Agent* agent);

	float getDamage() const { return _damage; }

	glm::vec2 getPosition() const { return _position; }

private:

	bool collideWithWorld(const std::vector<std::string>& levelData);

	glm::vec2 _position;
	glm::vec2 _direction;
	int _damage;
	float _speed;
};

