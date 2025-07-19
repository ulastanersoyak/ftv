cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()

project(
  ftv
  VERSION 1.0.0
  LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# default to debug, but let -DCMAKE_BUILD_TYPE=Release override it
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      Debug
      CACHE STRING "build type" FORCE)
endif()

option(FTV_BUILD_TESTS "build the test suite" ON)

add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
  -Werror
  -Wunused
  -Wuninitialized
  -Wshadow
  -Wconversion
  -Wsign-conversion
  -Wfloat-equal
  -Wold-style-cast
  -Wformat=2
  -Wvla
  -Wsuggest-override
  -Wnon-virtual-dtor)

# warnings only gcc knows about, clang would reject them under -Werror
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
    -Wuseless-cast
    -Weffc++
    -Wvexing-parse
    -Wsuggest-final-methods
    -Wsuggest-final-types
    -Wctor-dtor-privacy)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
    add_compile_options(-Wnrvo)
  endif()
endif()

add_subdirectory(src)

if(FTV_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
