#include "IOManager.h"

#include <fstream>

namespace Kengine
{
	bool IOManager::readFileToBuffer(std::string filePath, std::vector<unsigned char>& buffer)
	{
		std::ifstream file(filePath, std::ios::binary);
		if(file.fail())
		{
			perror(filePath.c_str());
			return false;
		}

		// Seek to the end
		file.seekg(0, std::ios::end);

		// Get the file size
		int fileSize = file.tellg();

		// Seek to the beginning
		file.seekg(0, std::ios::beg);

		// We don't care about the file header
		fileSize -= file.tellg();

		buffer.resize(fileSize);
		file.read((char *)&(buffer[0]), fileSize);

		file.close();

		return true;
	}
}