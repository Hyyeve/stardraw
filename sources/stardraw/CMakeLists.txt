add_library(stardraw STATIC)

target_sources(stardraw PRIVATE

        internal/window.cpp
        internal/shaders.cpp
        internal/glfw_window.hpp internal/glfw_window.cpp

        gl45/commands_impl.cpp
        gl45/render_context.hpp gl45/render_context.cpp
        gl45/object_states.hpp gl45/object_states.cpp
        gl45/types.hpp
        gl45/window.hpp gl45/window.cpp

        vk13/Device.hpp vk13/Device.cpp
        vk13/simple_window.hpp vk13/simple_window.cpp
)

target_sources(stardraw PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/render_context.hpp
        api/types.hpp
        api/polymorphic_ptr.hpp
        api/descriptors.hpp
        api/commands.hpp
        api/window.hpp
        api/shaders.hpp
        api/shader_parameter_value.hpp
)

target_include_directories(stardraw PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/stardraw>
        ${Vulkan_INCLUDE_DIRS}
)

target_compile_options(stardraw PUBLIC /EHsc -Wno-microsoft-extra-qualification)

set_target_properties(stardraw PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(stardraw PRIVATE ${OPENGL_INCLUDE_DIR})

target_link_libraries(stardraw PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(stardraw PRIVATE TracyClient)
target_link_libraries(stardraw PRIVATE glad)
target_link_libraries(stardraw PRIVATE slang::slang)
target_link_libraries(stardraw PRIVATE Vulkan::Vulkan)
target_link_libraries(stardraw PRIVATE glfw)
target_link_libraries(stardraw PRIVATE spirv-cross-glsl)
