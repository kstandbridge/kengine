#include "Errors.h"

#include <cstdlib>

#include <iostream>
#include <SDL/SDL.h>

namespace Kengine
{
	void fatalError(std::string errorString)
	{
		std::cout << errorString << std::endl;
		std::cout << "Enter any key to quit...";
		getchar();
		SDL_Quit();
		exit(1);
	}
}