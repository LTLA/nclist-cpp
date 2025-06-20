cmake_minimum_required(VERSION 3.24)

project(nclist
    VERSION 0.2.0
    DESCRIPTION "Nested containment lists in C++"
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

# Installing for find_package.
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ltla_nclist)

install(TARGETS nclist 
    EXPORT nclistTargets)

install(EXPORT nclistTargets
    FILE ltla_nclistTargets.cmake
    NAMESPACE ltla::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltla_nclist)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/ltla_nclistConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltla_nclist)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ltla_nclistConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ltla_nclistConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ltla_nclistConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltla_nclist)
