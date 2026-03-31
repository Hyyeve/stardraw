add_library(stardraw STATIC)

target_sources(stardraw PRIVATE

        internal/shaders.cpp
        internal/render_context.cpp
        internal/memory_transfer.cpp

        gl45/gl_headers.hpp
        gl45/common.hpp
        gl45/api_conversion.hpp
        gl45/render_context.hpp gl45/render_context.cpp
        gl45/object_creation.cpp gl45/commands.cpp
        gl45/memory_transfers.cpp gl45/state_binding.cpp

        gl45/memory_barrier_controller.hpp
        gl45/staging_buffer_uploader.hpp gl45/staging_buffer_uploader.cpp

        gl45/object_states/buffer_state.hpp gl45/object_states/buffer_state.cpp
        gl45/object_states/draw_specification_state.hpp gl45/object_states/draw_specification_state.cpp
        gl45/object_states/shader_state.hpp gl45/object_states/shader_state.cpp
        gl45/object_states/texture_state.hpp gl45/object_states/texture_state.cpp
        gl45/object_states/vertex_specification_state.hpp gl45/object_states/vertex_specification_state.cpp
        gl45/object_states/texture_sampler_state.hpp gl45/object_states/texture_sampler_state.cpp
        gl45/object_states/framebuffer_state.hpp gl45/object_states/framebuffer_state.cpp

        vk13/Device.hpp vk13/Device.cpp
        vk13/simple_window.hpp vk13/simple_window.cpp
)

target_sources(stardraw PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/render_context.hpp
        api/common.hpp
        api/descriptors.hpp
        api/commands.hpp
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

target_link_libraries(stardraw PUBLIC starlib)
target_link_libraries(stardraw PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(stardraw PRIVATE TracyClient)
target_link_libraries(stardraw PRIVATE glad)
target_link_libraries(stardraw PRIVATE slang::slang)
target_link_libraries(stardraw PRIVATE Vulkan::Vulkan)
target_link_libraries(stardraw PRIVATE spirv-cross-glsl)
