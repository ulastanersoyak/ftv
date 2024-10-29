add_library(lib STATIC ./file/file.cpp)
target_include_directories(lib PRIVATE ${CMAKE_SOURCE_DIR}/includes)

add_executable(main main.cpp)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/includes)
target_link_libraries(main lib)
