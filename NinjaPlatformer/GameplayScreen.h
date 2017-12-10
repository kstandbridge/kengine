#pragma once

#include <Kengine/IGameScreen.h>

class GameplayScreen : public Kengine::IGameScreen
{
public:
	GameplayScreen();
	~GameplayScreen();

	int getNextScreenIndex() const override;
	int getPreviousScreenIndex() const override;
	void build() override;
	void destory() override;
	void onEntry() override;
	void onExit() override;
	void update() override;
	void draw() override;
private:
	void checkInput();
};

