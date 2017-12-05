#pragma once

#include <glm/glm.hpp>
#include <Kengine/SpriteBatch.h>

class Bullet
{
public:
	Bullet(const glm::vec2& position, const glm::vec2& direction, const float speed, const int lifeTime)
		: _lifeTime(lifeTime),
		  _speed(speed),
		  _direction(direction),
		  _position(position)
	{
	}

	~Bullet();

	void draw(Kengine::SpriteBatch& spriteBatch);
	
	// Returns true when we are out of life
	bool update();

private:
	int _lifeTime;
	float _speed;
	glm::vec2 _direction;
	glm::vec2 _position;
};

