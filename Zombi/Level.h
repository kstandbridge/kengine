#pragma once

#include <Kengine/SpriteBatch.h>

#include <vector>
#include <string>

const int TILE_WIDTH = 64;

class Level
{
public:
	// Load the level
	Level(const std::string& fileName);
	~Level();

	void draw();

	// Getters
	int getWidth() const { return _levelData[0].size(); }
	int getHeight() const { return _levelData.size(); }
	int getNumHumans() const { return _numHumans; }
	const std::vector<std::string>& getLevelData() const { return _levelData; }
	const glm::vec2& getStartPlayerPos() const { return _startPlayerPos; }
	const std::vector<glm::ivec2>& getZombiStartPositions() const { return _zombitStartPositions; }

private:
	std::vector<std::string> _levelData;
	int _numHumans;
	Kengine::SpriteBatch _spriteBatch;

	glm::vec2 _startPlayerPos;
	std::vector<glm::ivec2> _zombitStartPositions;
};

