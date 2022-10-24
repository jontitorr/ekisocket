cmake_minimum_required(VERSION 3.15)

project(
    "ekisocket"
    VERSION 0.1.0
    LANGUAGES CXX
)

include(cmake/StandardSettings.cmake)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(NOT DEFINED ${PROJECT_NAME}_MASTER_PROJECT)
    set(${PROJECT_NAME}_MASTER_PROJECT OFF)

    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(${PROJECT_NAME}_MASTER_PROJECT ON)
    endif()
endif()

message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

if(UNIX)
    add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>")
endif()

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(
        FATAL_ERROR
            "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.\n"
    )
endif()

include(cmake/SourcesAndHeaders.cmake)

add_library(${PROJECT_NAME} ${headers} ${sources})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

include(cmake/Dependencies.cmake)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<INSTALL_INTERFACE:include>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})

target_compile_definitions(
    ${PROJECT_NAME}
    PUBLIC
        $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:${PROJECT_NAME_UPPERCASE}_STATIC_DEFINE>
)

target_include_directories(
    ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(${PROJECT_NAME}_BUILD_EXAMPLES AND ${PROJECT_NAME}_MASTER_PROJECT)
    add_subdirectory(examples)
endif()

if(${PROJECT_NAME}_BUILD_TESTS AND ${PROJECT_NAME}_MASTER_PROJECT)
    enable_testing()
    add_subdirectory(test)
endif()

if(${PROJECT_NAME}_INSTALL)
    include(GNUInstallDirs)
    install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES
        DESTINATION include
        PUBLIC_HEADER DESTINATION include
    )

    install(
        EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/version.hpp.in
        include/${PROJECT_NAME_LOWERCASE}/version.hpp @ONLY
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.hpp
        DESTINATION include/${PROJECT_NAME_LOWERCASE}
    )

    install(DIRECTORY include/${PROJECT_NAME_LOWERCASE} DESTINATION include)

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
        ${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
                  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

    install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME_LOWERCASE}_export.h
            DESTINATION include
    )
endif()
