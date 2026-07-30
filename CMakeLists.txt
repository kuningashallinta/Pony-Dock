cmake_minimum_required(VERSION 3.28)

project(PonyDock CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imgui STATIC
	vendor/imgui/imgui.cpp
	vendor/imgui/imgui_demo.cpp
	vendor/imgui/imgui_draw.cpp
	vendor/imgui/imgui_tables.cpp
	vendor/imgui/imgui_widgets.cpp
	vendor/imgui/backends/imgui_impl_win32.cpp
	vendor/imgui/backends/imgui_impl_dx11.cpp)

target_include_directories(imgui PUBLIC
	vendor/imgui
	vendor/imgui/backends)

target_link_libraries(imgui PUBLIC d3d11 dxgi)

add_subdirectory(Client)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Client)
