#include "TextureCache.h"
#include "ImageLoader.h"

namespace Kengine
{
	TextureCache::TextureCache()
	{
	}


	TextureCache::~TextureCache()
	{
	}

	GLTexture TextureCache::getTexture(std::string texturePath)
	{
		auto mit = _textureMap.find(texturePath);

		if(mit == _textureMap.end())
		{
			GLTexture newTexture = ImageLoader::LoadPNG(texturePath);

			_textureMap.insert(make_pair(texturePath, newTexture));

			return newTexture;
		}

		return mit->second;
	}
}