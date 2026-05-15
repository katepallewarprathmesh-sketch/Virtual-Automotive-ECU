add_executable(hsm_service main.cpp)
target_link_libraries(hsm_service PRIVATE common_hsm Threads::Threads)
target_include_directories(hsm_service PRIVATE ${CMAKE_SOURCE_DIR}/common/hsm)
