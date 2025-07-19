add_executable(ftv_tests tests.cpp)
target_link_libraries(ftv_tests PRIVATE ftv_lib)
add_test(NAME ftv_tests COMMAND ftv_tests)
