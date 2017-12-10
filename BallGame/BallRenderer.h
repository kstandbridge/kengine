#pragma once

#include <Kengine/SpriteBatch.h>
#include <Kengine/GLSLProgram.h>
#include <vector>
#include <memory>
#include "Ball.h"

class BallRenderer
{
public:
	virtual void renderBalls(Kengine::SpriteBatch& spriteBatch,
	                         const std::vector<Ball>& balls,
	                         const glm::mat4& projectionMatrix);
protected:
	std::unique_ptr<Kengine::GLSLProgram> m_program = nullptr;
};

class MomentumBallRenderer : public BallRenderer
{
public:
	void renderBalls(Kengine::SpriteBatch& spriteBatch,
	                 const std::vector<Ball>& balls,
	                 const glm::mat4& projectionMatrix) override;
};

class VelocityBallRenderer : public BallRenderer
{
public:

	VelocityBallRenderer(int screenWidth, int screenHeight);
	void renderBalls(Kengine::SpriteBatch& spriteBatch,
	                 const std::vector<Ball>& balls,
	                 const glm::mat4& projectionMatrix) override;
private:
	int m_screenWidth;
	int m_screenHeight;
};

class TrippyBallRenderer : public BallRenderer
{
public:
	TrippyBallRenderer(int screenWidth, int screenHeight);

	void renderBalls(Kengine::SpriteBatch& spriteBatch,
		const std::vector<Ball>& balls,
		const glm::mat4& projectionMatrix) override;
private:
    int m_screenWidth;
    int m_screenHeight;
    float m_time = 0.0f;
};