add_library(common_scheduler STATIC TaskScheduler.cpp)
target_include_directories(common_scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common_scheduler PUBLIC Threads::Threads)
