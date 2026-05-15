add_executable(unit_tests unit_tests.cpp)
target_link_libraries(unit_tests PRIVATE common_can common_hsm common_uds Threads::Threads)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/common/can ${CMAKE_SOURCE_DIR}/common/hsm ${CMAKE_SOURCE_DIR}/common/uds)
add_test(NAME unit_tests COMMAND unit_tests)
