find_package(OpenSSL REQUIRED)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

file(
  GLOB DIRS
  LIST_DIRECTORIES true
  "${CMAKE_CURRENT_SOURCE_DIR}/*")

set(ALL_SOURCES)

foreach(DIR ${DIRS})
  if(IS_DIRECTORY ${DIR})
    file(GLOB_RECURSE SOURCES "${DIR}/*.cpp")
    list(APPEND ALL_SOURCES ${SOURCES})
    message(STATUS "found sources in: ${DIR}")
    message(STATUS "files: ${SOURCES}")
  endif()
endforeach()

add_library(ftv_lib STATIC ${ALL_SOURCES})

target_link_libraries(ftv_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto
                                     ${OpenCV_LIBS})

target_include_directories(ftv_lib PUBLIC "${CMAKE_SOURCE_DIR}/includes")

add_executable(ftv main.cpp)
target_link_libraries(ftv PRIVATE ftv_lib)
