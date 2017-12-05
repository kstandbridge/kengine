#pragma once

#include <string>
#include <GL/glew.h>

namespace Kengine
{
	// This class handles the compilation, linking and usage of a GLSL shader program.
	// Reference: https://www.khronos.org/opengl/wiki/Shader_Compilation
	class GLSLProgram
	{
	public:
		GLSLProgram();
		~GLSLProgram();

		void compileShaders(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath);

		void linkShaders();

		void addAttribute(const std::string& attributeName);

		GLint getUniformLocation(const std::string& uniformName);

		void use();
		void unuse();
	private:

		int _numAttributes;

		void compileShader(const std::string& filePath, GLuint id);

		GLuint _programID;

		GLuint _vertexShaderID;
		GLuint _fragmentShaderID;
	};
}