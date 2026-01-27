cmake_minimum_required(VERSION 3.28)
set(CMAKE_DEBUG_POSTFIX "-debug")

include(cmake/Config.cmake)

project(${H_PROJECT_NAME} VERSION ${H_PROJECT_VERSION} DESCRIPTION ${H_PROJECT_DESCRIPTION} LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

include(cmake/Dependencies.cmake)
include(cmake/Sources.cmake)

set_target_properties(${H_TARGETS} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
add_custom_target(${H_PROJECT_NAME}InstallProject "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target install)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${H_PROJECT_NAME}Config-version.cmake
        COMPATIBILITY AnyNewerVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${H_PROJECT_NAME}Config-version.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${H_PROJECT_NAME}
)

install(
        TARGETS ${H_TARGETS}
        EXPORT ProjectTargets
        FILE_SET HEADERS
)

install(
        EXPORT ProjectTargets
        NAMESPACE ${H_PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${H_PROJECT_NAME}
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${H_PROJECT_NAME}Config-version.cmake
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${H_PROJECT_NAME}
)

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Export.cmake
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${H_PROJECT_NAME}
        RENAME ${H_PROJECT_NAME}Config.cmake
)

include(cmake/PackageConfigs.cmake)