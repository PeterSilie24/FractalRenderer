cmake_minimum_required(VERSION 3.9)

project(FractalRenderer)

add_subdirectory("extern")

add_executable(FractalRenderer
    "source/Main.cpp"
    "source/Includes.hpp"
    "source/OpenGLHelper.hpp"
    "source/Fractals.hpp"
    )

target_link_libraries(FractalRenderer glm glfw glad imgui stb)
