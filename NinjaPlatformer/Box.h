#pragma once

#include <Box2D/Box2D.h>
#include <glm/glm.hpp>
#include <Kengine/Vertex.h>
#include <Kengine/SpriteBatch.h>
#include <Kengine/GLTexture.h>

class Box
{
public:
	Box();
	~Box();

	void init(b2World* world, 
			  const glm::vec2 position, 
			  const glm::vec2 dimensions, 
			  Kengine::GLTexture texture, 
			  Kengine::ColorRGBA8 color, 
			  bool fixedRotation, 
			  glm::vec4 uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

	void draw(Kengine::SpriteBatch& spriteBatch);

	b2Body* getBody() const { return m_body; }
	b2Fixture* getFixture() const { return m_fixture; }
	const glm::vec2& getDimensions() const {return m_dimensions; }
	const Kengine::ColorRGBA8 getColor() const { return m_color; }
private:
	glm::vec4 m_uvRect;
	b2Body* m_body = nullptr;
	b2Fixture* m_fixture = nullptr;
	glm::vec2 m_dimensions;
	Kengine::ColorRGBA8 m_color;
	Kengine::GLTexture m_texture;
};
