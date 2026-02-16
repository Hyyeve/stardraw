add_executable(stardraw-demo)

target_sources(stardraw-demo PRIVATE
    main.cpp
)

target_link_libraries(stardraw-demo PRIVATE stardraw)
target_link_libraries(stardraw-demo PRIVATE slang::slang)