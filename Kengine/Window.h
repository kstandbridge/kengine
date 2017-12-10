#pragma once
#include <SDL/SDL.h>
#include <GL/glew.h>
#include <string>

namespace Kengine
{
	enum WindowFlags
	{
		INVISIBLE = 0x1, 
		FULL_SCREEN = 0x2, 
		BORDERLESS = 0x4
	};

	class Window
	{
	public:
		Window();
		~Window();

		int create(std::string windowName, int screenWidth, int screenHeight, unsigned int currentFlags);

		void swapBuffer();

		int getScreenWidth() const { return _screenWidth; }
		int getScreenHeight() const { return _screenHeight; }
	private:
		SDL_Window* _sdlWindow;
		int _screenWidth, _screenHeight;
	};
}