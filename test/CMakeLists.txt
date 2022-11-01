cmake_minimum_required(VERSION 3.15)

project(${CMAKE_PROJECT_NAME}Tests LANGUAGES CXX)

cpmfindpackage(
    NAME
    Catch2
    GITHUB_REPOSITORY
    catchorg/Catch2
    GIT_TAG
    "v3.1.1"
    OPTIONS
    "CATCH_BUILD_TESTING OFF"
    "CATCH_INSTALL_DOCS OFF"
    "CATCH_INSTALL_HELPERS OFF"
)

file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "src/*.cpp")

foreach(file ${test_sources})
    get_filename_component(test_name ${file} NAME_WE)
    add_executable(${test_name}_Tests ${file})

    target_link_libraries(
        ${test_name}_Tests PRIVATE Catch2::Catch2 ${CMAKE_PROJECT_NAME}
    )

    add_test(NAME ${test_name} COMMAND ${test_name}_Tests)
endforeach()

message(STATUS "Finished adding unit tests for ${CMAKE_PROJECT_NAME}.")
