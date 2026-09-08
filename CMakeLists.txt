cmake_minimum_required(VERSION 3.24)

project(
    network_diagnostic_cli
    VERSION 0.2.0
    DESCRIPTION "Cross-platform network diagnostic CLI for DNS, TCP, HTTP/HTTPS, latency, and environment checks"
    LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

option(ZTXDIAG_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(ZTXDIAG_CORE_SOURCES
    src/diagnostics.cpp
    src/format.cpp
    src/http_common.cpp
)

if(WIN32)
    list(APPEND ZTXDIAG_CORE_SOURCES src/http_winhttp.cpp)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBCURL REQUIRED IMPORTED_TARGET libcurl)
    list(APPEND ZTXDIAG_CORE_SOURCES src/http_curl.cpp)
endif()

add_library(ztxdiag_core ${ZTXDIAG_CORE_SOURCES})
add_library(ztxdiag::core ALIAS ztxdiag_core)

target_compile_features(ztxdiag_core PUBLIC cxx_std_20)

target_include_directories(ztxdiag_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(ztxdiag_core PUBLIC ws2_32 winhttp)
else()
    target_link_libraries(ztxdiag_core PUBLIC PkgConfig::LIBCURL)
endif()

if(MSVC)
    target_compile_options(ztxdiag_core PRIVATE
        /W4
        /permissive-
        /EHsc
        $<$<BOOL:${ZTXDIAG_WARNINGS_AS_ERRORS}>:/WX>
    )
else()
    target_compile_options(ztxdiag_core PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<BOOL:${ZTXDIAG_WARNINGS_AS_ERRORS}>:-Werror>
    )
endif()

add_executable(ztxdiag app/main.cpp)
target_link_libraries(ztxdiag PRIVATE ztxdiag::core)
target_compile_features(ztxdiag PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(ztxdiag PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(ztxdiag PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(
    TARGETS ztxdiag ztxdiag_core
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
