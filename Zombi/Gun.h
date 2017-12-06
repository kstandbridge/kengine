#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <Kengine/AudioEngine.h>

#include "Bullet.h"

class Gun
{
public:
	Gun(const std::string& name,
		const int fireRate, 
		const int bulletsPerShot, 
		const float spread, 
		const float bulletDamage,
		const float bulletSpeed,
		Kengine::SoundEffect fireEffect);
	~Gun();

	void update(bool isMouseDown, const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets, float deltaTime);

private:

	Kengine::SoundEffect m_fireEffect;

	void fire(const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets);

	std::string _name;

	int _fireRate; 

	int _bulletsPerShot;

	float _spread; 

	float _bulletDamage;

	float _bulletSpeed;
	
	float _frameCounter;

};

