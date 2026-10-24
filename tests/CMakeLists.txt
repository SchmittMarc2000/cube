add_executable(opencube_tests test_main.cpp)
target_link_libraries(opencube_tests PRIVATE ocnet ocbase)
add_test(NAME opencube_tests COMMAND opencube_tests)
