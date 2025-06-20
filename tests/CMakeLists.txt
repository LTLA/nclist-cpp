include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/16d4f8eff6d7cefca6975d82a53f8fc995a6feb7.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Avoid installing GoogleTest when installing this project.
option(INSTALL_GTEST "Enable installation of googletest." OFF)

FetchContent_MakeAvailable(googletest)

enable_testing()

option(CODE_COVERAGE "Enable coverage testing" OFF)
set(DO_CODE_COVERAGE OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(DO_CODE_COVERAGE ON)
endif()

include(GoogleTest)

add_executable(
    libtest
    src/overlaps_any.cpp
    src/overlaps_within.cpp
    src/overlaps_extend.cpp
    src/overlaps_equal.cpp
    src/overlaps_start.cpp
    src/overlaps_end.cpp
    src/build.cpp
)

target_link_libraries(
    libtest
    gtest_main
    nclist
)

target_compile_options(libtest PRIVATE -Wall -Werror -Wextra -Wpedantic)

if(DO_CODE_COVERAGE)
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
endif()

gtest_discover_tests(libtest)
