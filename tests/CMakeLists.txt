add_executable(scheduler_tests scheduler_tests.cpp)

target_link_libraries(scheduler_tests PRIVATE scheduler)

add_test(NAME scheduler_tests COMMAND scheduler_tests)
