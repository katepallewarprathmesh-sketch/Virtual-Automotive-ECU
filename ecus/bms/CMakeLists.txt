add_executable(bms_ecu main.cpp)
target_link_libraries(bms_ecu PRIVATE common_can common_crypto common_hsm common_scheduler common_uds Threads::Threads)
target_include_directories(bms_ecu PRIVATE ${CMAKE_SOURCE_DIR}/common/can ${CMAKE_SOURCE_DIR}/common/crypto ${CMAKE_SOURCE_DIR}/common/hsm ${CMAKE_SOURCE_DIR}/common/scheduler ${CMAKE_SOURCE_DIR}/common/uds)
