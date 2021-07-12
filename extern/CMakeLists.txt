cmake_minimum_required(VERSION 3.9)

add_subdirectory("glm")
add_subdirectory("glfw")
add_subdirectory("glad")

add_definitions (-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
add_library (imgui STATIC "imgui/imgui.cpp" "imgui/imgui.h" "imgui/imconfig.h" "imgui/imgui_demo.cpp" "imgui/imgui_draw.cpp" "imgui/imgui_internal.h" "imgui/imgui_tables.cpp" "imgui/imgui_widgets.cpp" "imgui/imstb_rectpack.h" "imgui/imstb_textedit.h" "imgui/imstb_truetype.h" "imgui/backends/imgui_impl_glfw.h" "imgui/backends/imgui_impl_glfw.cpp" "imgui/backends/imgui_impl_opengl3.h" "imgui/backends/imgui_impl_opengl3.cpp")
target_link_libraries (imgui glad glfw)
target_include_directories (imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui" "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "stb")
