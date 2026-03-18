add_library(starwin STATIC)

target_sources(starwin PRIVATE
        internal/glfw_window.hpp internal/glfw_window.cpp
        internal/gl45_window.hpp internal/gl45_window.cpp
        internal/window.cpp
)

target_sources(starwin PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/window.hpp
)

target_link_libraries(starwin PUBLIC starlib)
target_link_libraries(starwin PRIVATE glfw)
target_link_libraries(starwin PRIVATE Tracy::TracyClient)
