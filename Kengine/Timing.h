#pragma once

namespace Kengine
{
	class FpsLimiter
	{
	public:
		FpsLimiter();
		void init(float targetFPS);

		void setMaxFPS(float targetFPS);

		void begin();
		
		// End will return the current FPS
		float end();
	private:
		void calculateFPS();

		float _fps;
		float _maxFPS;
		float _frameTime;
		unsigned int _startTicks;
	};

	
}
