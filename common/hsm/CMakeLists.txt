add_library(common_hsm STATIC HsmEmulator.cpp HsmService.cpp HsmClient.cpp)
target_include_directories(common_hsm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common_hsm PUBLIC OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
