#include "Gun.h"

#include <random>
#include <ctime>

#include <glm/gtx/rotate_vector.hpp>

Gun::Gun(const std::string& name, 
		 const int fireRate, 
		 const int bulletsPerShot, 
		 const float spread,
		 const float bulletDamage, 
		 const float bulletSpeed,
		 Kengine::SoundEffect fireEffect)
	: _name(name),
	  _fireRate(fireRate),
	  _bulletsPerShot(bulletsPerShot),
	  _spread(spread),
	  _bulletDamage(bulletDamage), 
	  _bulletSpeed(bulletSpeed),
	  _frameCounter(0), 
	  m_fireEffect(fireEffect)
{
}

void Gun::update(bool isMouseDown, const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets, float deltaTime)
{
	_frameCounter += 1.0f * deltaTime;
	if(_frameCounter >= _fireRate && isMouseDown)
	{
		fire(position, direction, bullets);
		_frameCounter = 0;
	}
}

void Gun::fire(const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets)
{
	static std::mt19937 randomEngine(time(nullptr));
	static std::uniform_real_distribution<float> randRotate(-_spread, _spread);

	m_fireEffect.play();
	
	for(int i = 0; i < _bulletsPerShot; i++)
	{
		glm::vec2 rand = glm::rotate(direction, randRotate(randomEngine) * (float)(3.14 / 180));
//		auto rand = glm::rotate(direction, randRotate(randomEngine));
		bullets.emplace_back(position,
							 rand,
							 _bulletDamage,
							 _bulletSpeed);
	}
}
