#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

#include "Vertex.h"

namespace Kengine
{
	enum class GlyphSortType
	{
		NONE, 
		FRONT_TO_BACK, 
		BACK_TO_FRONT, 
		TEXTURE
	};

	class Glyph
	{
	public:
		Glyph() {};
		Glyph(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, const ColorRGBA8& color);
		Glyph(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, const ColorRGBA8& color, float angle);

		GLuint texture;
		float depth;

		Vertex topLeft;
		Vertex bottomLeft;
		Vertex topRight;
		Vertex bottomRight;
	private:
		glm::vec2 rotatePoint(glm::vec2 point, float angle);
	};

	class RenderBatch
	{
	public:
		RenderBatch(GLuint offset, GLuint numVertices, GLuint texture)
			: offset{offset},
			  numVertices{numVertices},
			  texture{texture}
		{
		}

		GLuint offset;
		GLuint numVertices;
		GLuint texture;
	};

	class SpriteBatch
	{
	public:
		SpriteBatch();
		~SpriteBatch();

		void init();

		void begin(GlyphSortType sortType = GlyphSortType::TEXTURE);
		void end();

		void draw(const glm::vec4& destRect, 
				  const glm::vec4& uvRect,
				  GLuint texture, 
				  float depth, 
				  const ColorRGBA8& color);

		void draw(const glm::vec4& destRect, 
				  const glm::vec4& uvRect,
				  GLuint texture, 
				  float depth, 
				  const ColorRGBA8& color,
				  float angle);

		void draw(const glm::vec4& destRect, 
				  const glm::vec4& uvRect,
				  GLuint texture, 
				  float depth, 
				  const ColorRGBA8& color,
				  const glm::vec2& dir);

		void renderBatch();

	private:
		void createRenderBatches();
		void createVertexArray();
		void sortGlyphs();

		static bool compareFrontToBack(Glyph* a, Glyph* b);
		static bool compareBackToFront(Glyph* a, Glyph* b);
		static bool compareTexture(Glyph* a, Glyph* b);

		GLuint _vbo;
		GLuint _vao;
		GlyphSortType _sortType;


		std::vector<Glyph*> _glyphPointers; /// This is for sorting
		std::vector<Glyph> _glyphs; /// These are the actual glyphs
		std::vector<RenderBatch> _renderBatches;
	};
}
