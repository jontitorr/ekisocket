include(cmake/CPM.cmake)

cpmfindpackage(
    NAME
    OpenSSL
    GITHUB_REPOSITORY
    "Xminent/openssl-cmake"
    GIT_TAG
    "master"
    OPTIONS
    "WITH_APPS OFF"
)

if(OpenSSL_ADDED)
    add_library(${PROJECT_NAME}::ssl ALIAS ssl)
    add_library(${PROJECT_NAME}::crypto ALIAS crypto)
    target_link_libraries(${PROJECT_NAME} PRIVATE ssl crypto)
    install(
        TARGETS ssl crypto
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES
        DESTINATION include
        PUBLIC_HEADER DESTINATION include
    )
else()
    set_target_properties(
        OpenSSL::SSL OpenSSL::Crypto PROPERTIES IMPORTED_GLOBAL TRUE
    )
    add_library(${PROJECT_NAME}::ssl ALIAS OpenSSL::SSL)
    add_library(${PROJECT_NAME}::crypto ALIAS OpenSSL::Crypto)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

cpmfindpackage(NAME fmt GITHUB_REPOSITORY "fmtlib/fmt" GIT_TAG "9.1.0")

if(fmt_ADDED)
    add_library(${PROJECT_NAME}::fmt ALIAS fmt)
    target_link_libraries(${PROJECT_NAME} PRIVATE fmt)
    install(
        TARGETS fmt
        EXPORT ${PROJECT_NAME}Targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES
        DESTINATION include
        PUBLIC_HEADER DESTINATION include
    )
    install(DIRECTORY ${fmt_SOURCE_DIR}/include/ DESTINATION include)
else()
    set_target_properties(fmt::fmt PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(${PROJECT_NAME}::fmt ALIAS fmt::fmt)
    target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

if(WIN32 OR MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 crypt32)
endif()
