add_library(glad STATIC)

target_sources(glad PRIVATE
        src/glad.cpp
)

target_sources(glad PUBLIC FILE_SET HEADERS BASE_DIRS ${H_SOURCES_ROOT} FILES
        include/glad/glad.h
        include/KHR/khrplatform.h
)

target_include_directories(glad PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/glad>
)

set_target_properties(glad PROPERTIES LINKER_LANGUAGE CXX)