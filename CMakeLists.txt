cmake_minimum_required(VERSION 3.9)

project(FractalRenderer)

add_subdirectory("extern")

add_executable(FractalRenderer
    "source/Main.cpp"
    )

target_link_libraries(FractalRenderer glm glfw glad imgui)
