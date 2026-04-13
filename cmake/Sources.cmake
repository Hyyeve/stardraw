set(H_SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/sources)
set(H_TARGETS
        stardraw stardraw-demo glad
)

add_subdirectory_library(glad sources/glad)
add_subdirectory_library(stardraw sources/stardraw)
add_subdirectory_library(stardraw-demo sources/stardraw-demo)