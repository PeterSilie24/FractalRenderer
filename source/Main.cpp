#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cmath>

#define CODE(code) std::string(#code)

#define glCreate(type) [](){ GLuint id; glGen##type##s(1, &id); return id; }
#define glDelete(type) [](const GLuint id){ glDelete##type##s(1, &id); }

template <typename T>
class RAIIWrapper : protected std::shared_ptr<T>
{
public:
	template <typename Deleter>
	RAIIWrapper(const T& t, Deleter deleter) :
		std::shared_ptr<T>(new T(t), [=](const T* t) { deleter(*t); delete t; })
	{

	}

	operator T()
	{
		return this->operator*();
	}

	operator const T() const
	{
		return this->operator*();
	}

	operator bool() const
	{
		return this->operator bool();
	}
};

void glfwErrorCallback(int error, const char* description)
{
	std::cout << "GLFW-Error: " << description << std::endl;
}

std::vector<glm::vec3> sierpinskiIteration(const std::vector<glm::vec3>& vertices)
{
	std::vector<glm::vec3> newVertices;

	for (std::size_t i = 0; i < vertices.size() / 3; i++)
	{
		const glm::vec3& v1 = vertices[3 * i + 0];
		const glm::vec3& v2 = vertices[3 * i + 1];
		const glm::vec3& v3 = vertices[3 * i + 2];

		glm::vec3 offset1(0.0f, 0.0f, 0.0f);
		glm::vec3 offset2(0.0f, 0.5f, 0.0f);
		glm::vec3 offset3(0.5f, 0.0f, 0.0f);

		newVertices.push_back(0.5f * v1 + offset1);
		newVertices.push_back(0.5f * v2 + offset1);
		newVertices.push_back(0.5f * v3 + offset1);

		newVertices.push_back(0.5f * v1 + offset2);
		newVertices.push_back(0.5f * v2 + offset2);
		newVertices.push_back(0.5f * v3 + offset2);

		newVertices.push_back(0.5f * v1 + offset3);
		newVertices.push_back(0.5f * v2 + offset3);
		newVertices.push_back(0.5f * v3 + offset3);
	}

	return newVertices;
}

int main()
{
	RAIIWrapper<bool> glfwIsInit(static_cast<bool>(glfwInit()), [](const bool) { glfwTerminate(); });

	if (!glfwIsInit)
	{
		return -1;
	}

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	RAIIWrapper<GLFWwindow*> window(glfwCreateWindow(640, 480, "FractalRenderer", nullptr, nullptr), glfwDestroyWindow);

	if (!window)
	{
		return 1;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Glad-Error: Failed to initialize OpenGL context." << std::endl;

		return -1;
	}

	glfwSwapInterval(1);

	RAIIWrapper<GLuint> program(glCreateProgram(), glDeleteProgram);

	{
		GLint success;
		std::vector<GLchar> infoLog(1024);

		RAIIWrapper<GLuint> vertexShader(glCreateShader(GL_VERTEX_SHADER), glDeleteShader);

		auto vertexShaderCode = CODE(
			#version 330 core \n

			layout(location = 0) in vec3 position;

			void main()
			{
				gl_Position = vec4(position.xy * 2.0 - 1.0, 0.0, 1.0);
			}
		);

		auto vertexShaderSource = vertexShaderCode.c_str();

		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, infoLog.size(), nullptr, infoLog.data());

			std::cout << "GL-Error: Failed to compile vertex shader:\n" << infoLog.data() << std::endl;

			return -1;
		}

		RAIIWrapper<GLuint> fragmentShader(glCreateShader(GL_FRAGMENT_SHADER), glDeleteShader);

		auto fragmentShaderCode = CODE(
			#version 330 core \n

			out vec4 color;

			void main()
			{
				color = vec4(1.0f);
			}
		);

		auto fragmentShaderSource = fragmentShaderCode.c_str();

		glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, infoLog.size(), nullptr, infoLog.data());

			std::cout << "GL-Error: Failed to compile fragment shader:\n" << infoLog.data() << std::endl;

			return -1;
		}

		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);

		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success)
		{
			glGetProgramInfoLog(program, infoLog.size(), nullptr, infoLog.data());

			std::cout << "GL-Error: Failed to link program:\n" << infoLog.data() << std::endl;

			return -1;
		}
	}

	glUseProgram(program);

	std::vector<glm::vec3> vertices({
			{ 1.0f, 1.0f, 1.0f },
			{ 1.0f, 0.0f, 1.0f },
			{ 0.0f, 1.0f, 1.0f }
		});

	std::vector<glm::vec3> tmpVertices = vertices;

	for (int i = 0; i < 9; i++)
	{
		tmpVertices = sierpinskiIteration(tmpVertices);

		vertices.insert(vertices.end(), tmpVertices.begin(), tmpVertices.end());
	}

	RAIIWrapper<GLuint> vertexArray(glCreate(VertexArray)(), glDelete(VertexArray));

	glBindVertexArray(vertexArray);

	RAIIWrapper<GLuint> vertexBuffer(glCreate(Buffer)(), glDelete(Buffer));

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		glViewport(0, 0, width, height);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glBindVertexArray(vertexArray);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		glfwSwapBuffers(window);
	}

	return 0;
}
