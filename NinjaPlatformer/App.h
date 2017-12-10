#pragma once

#include <Kengine/IMainGame.h>
#include "GameplayScreen.h"

class App : public Kengine::IMainGame
{
public:
	App();
	~App();

	void onInit() override;
	void addScreens() override;
	void onExit() override;
private:
	std::unique_ptr<GameplayScreen> m_gameplayScreen = nullptr;
};

