file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
	"*.h"
	"*.cpp"
	"*.rc")

add_executable(Client WIN32 ${CLIENT_SOURCES})

set_property(TARGET Client PROPERTY CXX_STANDARD 23)

target_include_directories(Client PRIVATE .)

target_compile_definitions(Client PRIVATE NOMINMAX)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(Lua REQUIRED)

target_include_directories(Client PRIVATE ${LUA_INCLUDE_DIR})

target_link_libraries(Client PRIVATE
	imgui
	nlohmann_json::nlohmann_json
	EnTT::EnTT
	sol2::sol2
	${LUA_LIBRARIES}
	d3dcompiler
	dwmapi)

add_custom_command(TARGET Client POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_SOURCE_DIR}/Packs" "$<TARGET_FILE_DIR:Client>/Packs"
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
		"${CMAKE_SOURCE_DIR}/Scripts" "$<TARGET_FILE_DIR:Client>/Scripts"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/LICENSE" "${CMAKE_SOURCE_DIR}/CREDITS.md" "$<TARGET_FILE_DIR:Client>"
	VERBATIM)
