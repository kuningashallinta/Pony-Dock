cmake_minimum_required(VERSION 3.28)

set(PONYDOCK_TOOLCHAIN "${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake")

if(NOT "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "${PONYDOCK_TOOLCHAIN}")
	set(CMAKE_TOOLCHAIN_FILE "${PONYDOCK_TOOLCHAIN}"
		CACHE FILEPATH "" FORCE)
	unset(Z_VCPKG_ROOT_DIR CACHE)
	unset(VCPKG_ROOT_DIR CACHE)
endif()

project(PonyDock VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(imgui STATIC
	vendor/imgui/imgui.cpp
	vendor/imgui/imgui_demo.cpp
	vendor/imgui/imgui_draw.cpp
	vendor/imgui/imgui_tables.cpp
	vendor/imgui/imgui_widgets.cpp
	vendor/imgui/backends/imgui_impl_win32.cpp
	vendor/imgui/backends/imgui_impl_dx11.cpp
	vendor/imgui/misc/cpp/imgui_stdlib.cpp)

target_include_directories(imgui PUBLIC
	vendor/imgui
	vendor/imgui/backends
	vendor/imgui/misc/cpp)

target_link_libraries(imgui PUBLIC d3d11 dxgi)

add_subdirectory(Client)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Client)
