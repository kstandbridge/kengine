#include "BallRenderer.h"

void BallRenderer::renderBall(Kengine::SpriteBatch& spriteBatch,
                              Ball& ball)
{

	const glm::vec4 uvRect(0.0f, 0.0f, 1.0f, 1.0f);
	const glm::vec4 destRect(ball.position.x - ball.radius, 
							 ball.position.y - ball.radius, 
							 ball.radius * 2.0f, 
							 ball.radius * 2.0f);
	spriteBatch.draw(destRect, uvRect, ball.textureId, 0.0f, ball.color);
}
