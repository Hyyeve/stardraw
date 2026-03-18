add_library(starwin STATIC)

target_sources(starwin PRIVATE
        internal/glfw_window.hpp internal/glfw_window.cpp
        internal/gl45_window.hpp internal/gl45_window.cpp
        internal/window.cpp
        internal/virtual_controller.cpp
        internal/virtual_keyboard.cpp
        internal/virtual_mouse.cpp
)

target_sources(starwin PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/window.hpp
        api/input.hpp
)

target_link_libraries(starwin PUBLIC starlib)
target_link_libraries(starwin PRIVATE glfw)
target_link_libraries(starwin PRIVATE Tracy::TracyClient)
