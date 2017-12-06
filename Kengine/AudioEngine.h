#pragma once

#include <SDL/SDL_mixer.h>
#include <string>
#include <map>

namespace Kengine
{
	class SoundEffect
	{
	public:
		friend class AudioEngine;

		/**
		 * \brief Plays the sound effect file
		 * \param loops If -1 loop forever, otherwise play x times
		 */
		void play(int loops = 0);
	private:
		Mix_Chunk* m_chunk = nullptr;
	};

	class Music
	{
	public:
		friend class AudioEngine;

		/**
		 * \brief Plays the music file
		 * \param loops If 0 loop forever, otherwise play x times
		 */
		void play(int loops = 0);

		static void pause();
		static void stop();
		static void resume();
	private:
		Mix_Music* m_music = nullptr;
	};

	class AudioEngine
	{

	public:
		AudioEngine();
		~AudioEngine();

		void init();
		void destory();

		SoundEffect loadSoundEffect(const std::string& filePath);
		Music loadMusic(const std::string& filePath);

	private:
		std::map<std::string, Mix_Chunk*> m_effectMap;
		std::map<std::string, Mix_Music*> m_musicMap;
		
		bool m_isInitialized = false;
	};

}
