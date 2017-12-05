#include "Bullet.h"

#include <Kengine/ResourceManager.h>

Bullet::~Bullet()
{
}

void Bullet::draw(Kengine::SpriteBatch& spriteBatch)
{
	glm::vec4 uv(0.0f, 0.0f, 1.0f, 1.0f);
	static Kengine::GLTexture texture = Kengine::ResourceManager::getTexture("Textures/jimmyJump_pack/PNG/CharacterRight_Standing.png");
	Kengine::ColorRGBA8 color;
	color.r = 255;
	color.g = 255;
	color.b = 255;
	color.a = 255;

	glm::vec4 posAndSize = glm::vec4(_position.x, _position.y, 30, 30);

	spriteBatch.draw(posAndSize, uv, texture.id, 0.0f, color);
}

bool Bullet::update()
{
	_position += _direction * _speed;
	_lifeTime--;
	if(_lifeTime == 0)
	{
		return true;
	}
	return false;
}
