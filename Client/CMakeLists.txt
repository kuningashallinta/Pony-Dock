file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
	"*.h"
	"*.cpp")

add_executable(Client WIN32 ${CLIENT_SOURCES})

set_property(TARGET Client PROPERTY CXX_STANDARD 23)

target_include_directories(Client PRIVATE .)

target_link_libraries(Client PRIVATE imgui)
