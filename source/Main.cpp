#include "OpenGLHelper.hpp"
#include "Fractals.hpp"

void glfwErrorCallback(int error, const char* description)
{
	std::cout << "GLFW-Error: " << description << std::endl;
}

void APIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::cout << "GL-Message: " << message << std::endl;
}

std::shared_ptr<fractals::Fractal> fractal;

bool iterate = false;

int width = 1, height = 1;
bool moving = false;
glm::dvec2 pos(0.0);

fractals::Viewport viewport(0.0, 0.0, 0.0, 10.0);

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_F:
		{
			fractal = nullptr;

			fractal = std::make_shared<fractals::BarnsleyFern>(glm::ivec2(1080 * 8, 1920 * 8));

			break;
		}
		case GLFW_KEY_T:
		{
			fractal = nullptr;

			fractal = std::make_shared<fractals::SierpinskiTriangle>(glm::ivec2(1920 * 8, 1080 * 8));

			break;
		}
		case GLFW_KEY_SPACE:
		{
			if (fractal)
			{
				fractal->iterate();
			}

			break;
		}
		case GLFW_KEY_ENTER:
		{
			iterate = !iterate;

			break;
		}
		case GLFW_KEY_BACKSPACE:
		{
			if (fractal)
			{
				fractal->reset();
			}

			break;
		}
		}
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			moving = true;

			glfwGetCursorPos(window, &pos.x, &pos.y);
		}
		else if (action == GLFW_RELEASE)
		{
			moving = false;
		}
	}
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (moving)
	{
		glm::dvec2 newPos(xpos, ypos);

		glm::dvec2 offset = (newPos - pos) / glm::dvec2(width, height) * glm::dvec2(viewport.left - viewport.right, viewport.top - viewport.bottom);

		viewport.left += offset.x;
		viewport.right += offset.x;
		viewport.bottom += offset.y;
		viewport.top += offset.y;

		pos = newPos;
	}
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	double x = (viewport.left + viewport.right) / 2.0;
	double y = (viewport.bottom + viewport.top) / 2.0;

	double factor = glm::pow(1.05, -yoffset);

	viewport.right = (viewport.right - viewport.left) / 2.0;
	viewport.left = -viewport.right;

	viewport.top = (viewport.top - viewport.bottom) / 2.0;
	viewport.bottom = -viewport.top;

	viewport.viewport *= factor;

	viewport.left += x;
	viewport.right += x;
	viewport.bottom += y;
	viewport.top += y;
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	RAIIWrapper<GLFWwindow*> window(glfwCreateWindow(1920, 1080, "FractalRenderer", nullptr, nullptr), glfwDestroyWindow);

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

	glfwSetKeyCallback(window, keyCallback);

	glfwSetMouseButtonCallback(window, mouseButtonCallback);

	glfwSetCursorPosCallback(window, cursorPosCallback);

	glfwSetScrollCallback(window, scrollCallback);
	
	/*if (gl::extensionAvailable("GL_ARB_debug_output"))
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

		glDebugMessageCallbackARB(glErrorCallback, nullptr);
	}*/

	glfwSwapInterval(1);

	RAIIWrapper<GLuint> vertexArray(glCreate(VertexArray)(), glDelete(VertexArray));

	glBindVertexArray(vertexArray);

	try
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			if (iterate)
			{
				if (fractal)
				{
					fractal->iterate();
				}
			}

			glfwGetFramebufferSize(window, &width, &height);

			glViewport(0, 0, width, height);

			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			if (fractal)
			{
				double x = (viewport.left + viewport.right) / 2.0;

				viewport.right = (viewport.top - viewport.bottom) / 2.0;
				viewport.left = -viewport.right;

				double factor = double(width) / double(height);

				viewport.left *= factor;
				viewport.right *= factor;

				viewport.left += x;
				viewport.right += x;

				fractal->render(glm::ivec2(width, height), viewport);
			}

			glfwSwapBuffers(window);

			//std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	catch (const std::runtime_error & error)
	{
		std::cout << error.what() << std::endl;
	}

	return 0;
}
