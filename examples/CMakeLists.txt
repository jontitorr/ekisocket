cmake_minimum_required(VERSION 3.15)

project(${CMAKE_PROJECT_NAME}Examples LANGUAGES CXX)

file(GLOB_RECURSE examples_sources CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

foreach(file ${examples_sources})
    get_filename_component(example_name ${file} NAME_WE)
    get_filename_component(example_dir ${file} DIRECTORY)
    get_filename_component(example_dir ${example_dir} NAME)
    set(example_name "${example_dir}_${example_name}")

    add_executable(${example_name} ${file})
    target_link_libraries(${example_name} PRIVATE ${CMAKE_PROJECT_NAME})
endforeach()

message(STATUS "Finished adding examples for ${CMAKE_PROJECT_NAME}.")
