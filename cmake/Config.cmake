set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS -Wall)
set(CMAKE_CXX_FLAGS -Wexceptions)
set(CMAKE_CXX_FLAGS -Wno-c++98-compat) #Don't care
set(CMAKE_CXX_FLAGS -Wno-microsoft-extra-qualification) #I *want* my namespaced functions to be qualified, dangit

set(H_PROJECT_NAME
        stardraw
)
set(H_PROJECT_VERSION
        0.0.1
)
set(H_PROJECT_DESCRIPTION
        "Hyeve's low-level portable graphics abstraction"
)
