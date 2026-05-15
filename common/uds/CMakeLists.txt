add_library(common_uds STATIC UdsProtocol.cpp)
target_include_directories(common_uds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
