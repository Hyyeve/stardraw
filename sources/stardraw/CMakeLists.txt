add_library(stardraw STATIC)

target_sources(stardraw PRIVATE

        api/api_impl.cpp
        api/pipeline.cpp

        gl45/commands_impl.cpp
        gl45/gl45_impl.hpp gl45/gl45_impl.cpp
        gl45/object_states.hpp gl45/object_states.cpp
        gl45/types.hpp
)

target_sources(stardraw PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        api/pipeline.hpp
        api/types.hpp
        api/api_impl.hpp
        api/polymorphic_list.hpp

        api/descriptors.hpp
        api/commands.hpp
)

target_include_directories(stardraw PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/stardraw>
)

set_target_properties(stardraw PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(stardraw PRIVATE ${OPENGL_INCLUDE_DIR})

target_link_libraries(stardraw PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(stardraw PRIVATE TracyClient)
target_link_libraries(stardraw PRIVATE glad)
