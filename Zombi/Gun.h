#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Bullet.h"

class Gun
{
public:
	Gun(const std::string& name,
		const int fireRate, 
		const int bulletsPerShot, 
		const float spread, 
		const float bulletDamage,
		const float bulletSpeed);
	~Gun();

	void update(bool isMouseDown, const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets);

private:

	void fire(const glm::vec2& position, const glm::vec2& direction, std::vector<Bullet>& bullets);

	std::string _name;

	int _fireRate; 

	int _bulletsPerShot;

	float _spread; 

	float _bulletDamage;

	float _bulletSpeed;
	
	int _frameCounter;

};

