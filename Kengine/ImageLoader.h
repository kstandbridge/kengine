#pragma once
#include "GLTexture.h"

#include <string>
namespace Kengine
{
	class ImageLoader
	{
	public:
		static GLTexture LoadPNG(std::string filePath);
	};

}