file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
	"*.h"
	"*.cpp"
	"*.rc")

add_executable(Client WIN32 ${CLIENT_SOURCES})

set_property(TARGET Client PROPERTY CXX_STANDARD 23)

target_include_directories(Client PRIVATE .)

target_compile_definitions(Client PRIVATE
	NOMINMAX
	"PONYDOCK_PACKS_DIR=\"${CMAKE_SOURCE_DIR}/Packs\""
	"PONYDOCK_SCRIPTS_DIR=\"${CMAKE_SOURCE_DIR}/Scripts\"")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

target_include_directories(Client PRIVATE ${LUA_INCLUDE_DIR})

target_link_libraries(Client PRIVATE
	imgui
	imgui_texteditor
	nlohmann_json::nlohmann_json
	EnTT::EnTT
	sol2::sol2
	${LUA_LIBRARIES}
	d3dcompiler
	dwmapi)
