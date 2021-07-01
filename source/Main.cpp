#include "OpenGLHelper.hpp"
#include "Fractals.hpp"

void APIENTRY glfwErrorCallback(int error, const char* description)
{
	std::cout << "GLFW-Error: " << description << std::endl;
}

void APIENTRY glErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	std::cout << "GL-Message: " << message << std::endl;
}

std::shared_ptr<fractals::Fractal> fractal;

bool iterate = true;

int width = 1, height = 1;
bool moving = false;
glm::dvec2 pos(0.0);

bool windowFocused = false;
bool anyWindowFocused = false;

std::int32_t iterationsPerFrame = 1;

fractals::Viewport viewport(0.0, 0.0, 0.0, 10.0);

template <typename TFractal, typename... Arguments>
void setFractal(Arguments&&... arguments)
{
	fractal = nullptr;

	fractal = std::make_shared<TFractal>(std::forward<Arguments>(arguments)...);

	viewport = fractal->getPreferredViewport();

	iterationsPerFrame = fractal->getPreferredIterationsPerFrame();
}

struct FractalSelector
{
	std::string name;
	std::function<void()> select;

	template <typename TFractal, typename... Arguments>
	static FractalSelector create(const std::string& name, Arguments&&... arguments)
	{
		FractalSelector fractalSelector;
		fractalSelector.name = name;
		fractalSelector.select = std::function<void()>([=] { setFractal<TFractal>(arguments...); });

		return fractalSelector;
	}
};

std::vector<FractalSelector> fractalSelectors(
	{
		FractalSelector::create<fractals::BarnsleyFern>("Barnsley Fern", glm::ivec2(4096, 4096)),
		FractalSelector::create<fractals::SierpinskiTriangle>("Sierpinski Triangle", glm::ivec2(4096, 4096)),
		FractalSelector::create<fractals::Mandelbrot>("Mandelbrot", glm::ivec2(1920, 1080), viewport, 2),
	}
);

const FractalSelector* selectedFractalSelector = nullptr;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (!anyWindowFocused/* || windowFocused*/)
	{
		if (action == GLFW_PRESS)
		{
			switch (key)
			{
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
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (!anyWindowFocused)
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
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	glm::dvec2 newPos(xpos, ypos);

	if (!anyWindowFocused)
	{
		if (moving)
		{
			glm::dvec2 offset = (newPos - pos) / glm::dvec2(width, height) * glm::dvec2(viewport.left - viewport.right, viewport.top - viewport.bottom);

			viewport.left += offset.x;
			viewport.right += offset.x;
			viewport.bottom += offset.y;
			viewport.top += offset.y;
		}
	}

	pos = newPos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (!anyWindowFocused)
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
}

int main()
{
	RAIIWrapper<bool> glfwIsInit(static_cast<bool>(glfwInit()), [](const bool) { glfwTerminate(); });

	if (!glfwIsInit)
	{
		return -1;
	}

	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	RAIIWrapper<GLFWwindow*> window(glfwCreateWindow(800, 600, "Fractal Renderer", nullptr, nullptr), glfwDestroyWindow);

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

	IMGUI_CHECKVERSION();
	RAIIWrapper<ImGuiContext*> imGuiContext(ImGui::CreateContext(), ImGui::DestroyContext);

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15.0f, 15.0f);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5.0f, 5.0f);
	style->FrameRounding = 5.0f;

	style->IndentSpacing = 25.0f;
	style->ItemSpacing = ImVec2(10.0f, 7.5f);
	style->ItemInnerSpacing = ImVec2(10.0f, 7.5f);

	style->ScrollbarSize = 10.0f;
	style->ScrollbarRounding = 10.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 5.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.825f, 0.825f, 0.825f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.75f, 0.75f, 0.75f, 0.5f);

	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.65f);
	style->Colors[ImGuiCol_ChildBg] = style->Colors[ImGuiCol_WindowBg];

	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.8f);
	style->Colors[ImGuiCol_PopupBg] = style->Colors[ImGuiCol_MenuBarBg];
	
	style->Colors[ImGuiCol_Border] = ImVec4(0.75f, 0.75f, 0.75f, 0.25f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	style->Colors[ImGuiCol_Button] = ImVec4(0.125f, 0.125f, 0.125f, 0.75f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 0.9f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.5f, 0.5f, 0.5f, 0.9f);

	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.8f, 0.8f, 0.8f, 0.33f);

	style->Colors[ImGuiCol_SliderGrab] = style->Colors[ImGuiCol_ButtonActive];
	style->Colors[ImGuiCol_SliderGrabActive] = style->Colors[ImGuiCol_Button];

	style->Colors[ImGuiCol_FrameBg] = style->Colors[ImGuiCol_Button];
	style->Colors[ImGuiCol_FrameBgHovered] = style->Colors[ImGuiCol_ButtonHovered];
	style->Colors[ImGuiCol_FrameBgActive] = style->Colors[ImGuiCol_ButtonActive];

	style->Colors[ImGuiCol_Header] = style->Colors[ImGuiCol_FrameBg];
	style->Colors[ImGuiCol_HeaderHovered] = style->Colors[ImGuiCol_FrameBgHovered];
	style->Colors[ImGuiCol_HeaderActive] = style->Colors[ImGuiCol_FrameBgActive];

	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.6f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);

	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style->Colors[ImGuiCol_ScrollbarGrab] = style->Colors[ImGuiCol_Button];
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = style->Colors[ImGuiCol_ButtonHovered];
	style->Colors[ImGuiCol_ScrollbarGrabActive] = style->Colors[ImGuiCol_ButtonActive];

	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style->Colors[ImGuiCol_ResizeGripHovered] = style->Colors[ImGuiCol_ButtonHovered];
	style->Colors[ImGuiCol_ResizeGripActive] = style->Colors[ImGuiCol_ButtonActive];

	//io.Fonts->AddFontFromFileTTF("", 10);
	//io.Fonts->AddFontFromFileTTF("", 12);
	//io.Fonts->AddFontFromFileTTF("", 14);
	//io.Fonts->AddFontFromFileTTF("", 18);

	RAIIWrapper<bool> imGuiGlfwIsInit(ImGui_ImplGlfw_InitForOpenGL(window, true), [](const bool) { ImGui_ImplGlfw_Shutdown(); });
	RAIIWrapper<bool> imGuiOpenGLIsInit(ImGui_ImplOpenGL3_Init("#version 420"), [](const bool) { ImGui_ImplOpenGL3_Shutdown(); });

	try
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			if (iterate)
			{
				if (fractal)
				{
					fractal->iterate(iterationsPerFrame);
				}
			}

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();

			ImGui::NewFrame();

			ImGui::Begin("Fractal Renderer", nullptr, ImGuiWindowFlags_MenuBar);

			windowFocused = ImGui::IsWindowFocused();
			anyWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("Menu"))
				{
					if (ImGui::BeginMenu("Load Fractal"))
					{
						for (auto& fractalSelector : fractalSelectors)
						{
							if (ImGui::Selectable(fractalSelector.name.c_str(), &fractalSelector == selectedFractalSelector))
							{
								fractalSelector.select();

								selectedFractalSelector = &fractalSelector;
							}
						}

						ImGui::EndMenu();
					}

					ImGui::Separator();

					if (ImGui::MenuItem("Iteration Step", "SPACE", nullptr, !iterate))
					{
						if (fractal)
						{
							fractal->iterate();
						}
					}

					if (ImGui::MenuItem("Auto Iterate", "ENTER", &iterate))
					{

					}

					ImGui::Separator();

					if (ImGui::MenuItem("Reset", "BACKSPACE"))
					{
						if (fractal)
						{
							fractal->reset();
						}
					}

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("About"))
				{
					ImGui::MenuItem("About", nullptr, nullptr);

					ImGui::EndMenu();
				}

				ImGui::EndMenuBar();
			}

			if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderInt("Iterations per Frame", &iterationsPerFrame, 1, 1000);

				if (fractal)
				{
					fractal->options();
				}
			}

			if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::stringstream stream;

				stream << std::setprecision(16) << std::showpos;
				stream << "Left:   " << viewport.left << std::endl;
				stream << "Right:  " << viewport.right << std::endl;
				stream << "Bottom: " << viewport.bottom << std::endl;
				stream << "Top:    " << viewport.top;

				ImGui::Text(stream.str().c_str());

				if (fractal && ImGui::Button("Reset##Viewport"))
				{
					viewport = fractal->getPreferredViewport();
				}
			}

			if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Framerate: %.1f FPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			}

			ImGui::End();

			//ImGui::ShowDemoWindow(nullptr);

			ImGui::EndFrame();

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

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);

			//std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	catch (const std::runtime_error & error)
	{
		std::cout << error.what() << std::endl;
	}

	fractal = nullptr;

	return 0;
}
