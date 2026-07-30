add_executable(Client WIN32 main.cpp)

set_property(TARGET Client PROPERTY CXX_STANDARD 23)

target_link_libraries(Client PRIVATE imgui)
