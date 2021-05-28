#pragma once

#include "Includes.hpp"

#define CODE(code) std::string(#code)

#define glCreate(type) [](){ GLuint id; glGen##type##s(1, &id); return id; }
#define glDelete(type) [](const GLuint id){ glDelete##type##s(1, &id); }

template <typename T>
class RAIIWrapper : protected std::shared_ptr<T>
{
public:
	RAIIWrapper()
	{

	}

	RAIIWrapper(std::nullptr_t)
	{

	}

	template <typename Deleter>
	RAIIWrapper(const T& t, Deleter deleter) :
		std::shared_ptr<T>(new T(t), [=](const T* t) { deleter(*t); delete t; })
	{

	}

	bool valid() const
	{
		return this->get() != nullptr;
	}

	operator T()
	{
		if (this->valid())
		{
			return this->operator*();
		}

		return T(0);
	}

	operator const T() const
	{
		if (this->valid())
		{
			return this->operator*();
		}

		return T(0);
	}

	operator bool() const
	{
		return this->valid();
	}
};

namespace gl
{
	inline std::vector<std::string> getExtensions()
	{
		thread_local std::vector<std::string> extensions;

		if (extensions.empty())
		{
			GLint numExtensions;

			glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

			for (GLint i = 0; i < numExtensions; i++)
			{
				extensions.push_back(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
			}
		}

		return extensions;
	}

	inline bool extensionAvailable(const std::string& extension)
	{
		thread_local std::set<std::string> extensionsSet;

		if (extensionsSet.empty())
		{
			auto extensions = getExtensions();

			extensionsSet.insert(extensions.begin(), extensions.end());
		}

		return extensionsSet.count(extension) > 0;
	}

	inline void requireExtension(const std::string& extension)
	{
		if (!extensionAvailable(extension))
		{
			std::stringstream stream;

			stream << "GL-Error: The extension " << extension << " is required.";

			throw std::runtime_error(stream.str());
		}
	}

	inline RAIIWrapper<GLuint> compileAndLinkShaders(const std::string& vertexShaderCode, const std::string& fragmentShaderCode)
	{
		RAIIWrapper<GLuint> program(glCreateProgram(), glDeleteProgram);

		GLint success;
		std::vector<GLchar> infoLog(1024);

		RAIIWrapper<GLuint> vertexShader(glCreateShader(GL_VERTEX_SHADER), glDeleteShader);

		auto vertexShaderSource = vertexShaderCode.c_str();

		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(vertexShader, infoLog.size(), nullptr, infoLog.data());

			std::stringstream stream;

			stream << "GL-Error: Failed to compile vertex shader:\n" << infoLog.data();

			throw std::runtime_error(stream.str());
		}

		RAIIWrapper<GLuint> fragmentShader(glCreateShader(GL_FRAGMENT_SHADER), glDeleteShader);

		auto fragmentShaderSource = fragmentShaderCode.c_str();

		glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, infoLog.size(), nullptr, infoLog.data());

			std::stringstream stream;

			stream << "GL-Error: Failed to compile fragment shader:\n" << infoLog.data();

			throw std::runtime_error(stream.str());
		}

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success)
		{
			glGetProgramInfoLog(program, infoLog.size(), nullptr, infoLog.data());

			std::stringstream stream;

			stream << "GL-Error: Failed to link program:\n" << infoLog.data();

			throw std::runtime_error(stream.str());
		}

		return program;
	}
}
