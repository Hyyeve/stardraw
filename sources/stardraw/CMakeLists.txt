add_library(stardraw STATIC)

target_sources(stardraw PRIVATE

        internal/api_impl.cpp
        internal/pipeline.cpp
        internal/slang.cpp

        gl45/commands_impl.cpp
        gl45/gl45_impl.hpp gl45/gl45_impl.cpp
        gl45/object_states.hpp gl45/object_states.cpp
        gl45/types.hpp

        vk13/Device.hpp vk13/Device.cpp
        vk13/simple_window.hpp vk13/simple_window.cpp
)

target_sources(stardraw PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/pipeline.hpp
        api/types.hpp
        api/api_impl.hpp
        api/polymorphic_list.hpp
        api/descriptors.hpp
        api/commands.hpp

        internal/slang.hpp #TEMPORARILY EXPOSED
)

target_include_directories(stardraw PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/stardraw>
        ${Vulkan_INCLUDE_DIRS}
)

set_target_properties(stardraw PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(stardraw PRIVATE ${OPENGL_INCLUDE_DIR})

target_link_libraries(stardraw PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(stardraw PRIVATE TracyClient)
target_link_libraries(stardraw PRIVATE glad)
target_link_libraries(stardraw PRIVATE slang::slang)
target_link_libraries(stardraw PRIVATE Vulkan:Vulkan)
target_link_libraries(stardraw PRIVATE glfw)
