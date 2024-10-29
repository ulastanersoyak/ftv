find_package(OpenSSL REQUIRED)

add_library(ftv_lib file/file.cpp crypto/encrypt.cpp)

target_include_directories(ftv_lib PRIVATE ${CMAKE_SOURCE_DIR}/includes)

target_link_libraries(ftv_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)

add_executable(ftv main.cpp)

target_link_libraries(ftv PRIVATE ftv_lib)
target_include_directories(ftv PRIVATE ${CMAKE_SOURCE_DIR}/includes)
