file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
	"*.h"
	"*.cpp"
	"*.rc")

add_executable(Client WIN32 ${CLIENT_SOURCES})

set_property(TARGET Client PROPERTY CXX_STANDARD 23)

target_include_directories(Client PRIVATE .)

target_compile_definitions(Client PRIVATE
	NOMINMAX
	"PONYDOCK_PACKS_DIR=\"${CMAKE_SOURCE_DIR}/Packs\"")

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(Client PRIVATE imgui nlohmann_json::nlohmann_json)
