add_library(common_can STATIC CanSocket.cpp CanAuthenticator.cpp)
target_include_directories(common_can PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(common_can PUBLIC Threads::Threads)
