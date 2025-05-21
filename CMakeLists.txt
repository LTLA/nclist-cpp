cmake_minimum_required(VERSION 3.24)

project(nclist
    VERSION 0.1.0
    DESCRIPTION "C++ implementation of nested containment lists"
    LANGUAGES CXX)

add_library(nclist INTERFACE)
add_library(ltla::nclist ALIAS nclist)

target_compile_features(nclist INTERFACE cxx_std_17)

include(GNUInstallDirs)
target_include_directories(nclist INTERFACE 
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ltla_nclist>"
)

# Building the test-related machinery, if we are compiling this library directly.
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    option(NCLIST_TESTS "Build nclist's test suite." ON)
else()
    option(NCLIST_TESTS "Build nclist's test suite." OFF)
endif()

if(NCLIST_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(tests)
    endif() 
endif()
