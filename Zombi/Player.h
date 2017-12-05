#pragma once

#include "Human.h"
#include <Kengine/InputManager.h>
#include <Kengine/Camera2D.h>
#include "Bullet.h"

class Gun;

class Player : public Human
{
public:
	Player();
	~Player();

	void init(float speed, glm::vec2 pos, Kengine::InputManager* inputManager, Kengine::Camera2D* camera, std::vector<Bullet>* bullets);

	void addGun(Gun* gun);

	void update(const std::vector<std::string>& levelData,
				std::vector<Human*>& humans,
				std::vector<Zombie*>& zombies) override;
private:
	Kengine::InputManager* _inputManager;

	std::vector<Gun*> _guns;
	int _currentGunIndex;

	Kengine::Camera2D* _camera;
	std::vector<Bullet>* _bullets;
};

