#pragma once
#include <Kengine/SpriteBatch.h>
#include <BallGame/Ball.h>

class BallRenderer
{
public:
	void renderBall(Kengine::SpriteBatch& spriteBatch, Ball& ball);
};
