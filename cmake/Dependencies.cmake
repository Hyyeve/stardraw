# find_package(Dependency CONFIG REQUIRED)

find_package(OpenGL REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

include(FetchContent)

FetchContent_Declare(
        tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.12.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(tracy)
