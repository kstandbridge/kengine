#include "Sprite.h"
#include "Vertex.h"
#include "ResourceManager.h"

#include <cstddef>

namespace Kengine
{
	Sprite::Sprite()
	{
		_vboID = 0;
	}

	Sprite::~Sprite()
	{
		if(_vboID != 0)
		{
			glDeleteBuffers(1, &_vboID);
		}
	}

	void Sprite::init(float x, float y, float width, float height, std::string texturePath)
	{
		_x = x;
		_y = y;
		_width = width;
		_height = height;

		_texture = ResourceManager::getTexture(texturePath);

		// Generate the bugger if it hasn't already been generated
		if(_vboID == 0)
		{
			glGenBuffers(1, &_vboID);
		}

		// This array will hold our vertex data.
		// we need 6 vertices, and vertex has 2
		// floats for X and Y
		Vertex vertexData[6];

		// First Triangle
		vertexData[0].setPosition(x + width, y + height);
		vertexData[0].setUV(1.0f, 1.0f);
	
		vertexData[1].setPosition(x, y + height);
		vertexData[1].setUV(0.0f, 1.0f);
	
		vertexData[2].setPosition(x, y);
		vertexData[2].setUV(0.0f, 0.0f);
	
		// Second Triangle
		vertexData[3].setPosition(x, y);
		vertexData[3].setUV(0.0f, 0.0f);
	
		vertexData[4].setPosition(x + width, y);
		vertexData[4].setUV(1.0f, 0.0f);
		
		vertexData[5].setPosition(x + width, y + height);
		vertexData[5].setUV(1.0f, 1.0f);

		for(int i = 0; i < 6; i++)
		{
			vertexData[i].setColor(255, 0, 255, 255);
		}
	
		vertexData[1].setColor(0, 0, 255, 255);
		vertexData[4].setColor(0, 255, 0, 255);
	
		glBindBuffer(GL_ARRAY_BUFFER, _vboID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	// Draws the sprint to the screen
	void Sprite::draw()
	{
		glBindTexture(GL_TEXTURE_2D, _texture.id);

		// bind the buffer object
		glBindBuffer(GL_ARRAY_BUFFER, _vboID);

		

		// Draw the 6 vertices to the screen
		glDrawArrays(GL_TRIANGLES, 0, 6);
	
		// Disable the vertex attrib array. This is not optional.
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Unbind the VBO
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}