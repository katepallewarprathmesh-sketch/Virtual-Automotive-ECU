add_library(common_crypto STATIC SecureBoot.cpp)
target_include_directories(common_crypto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(common_crypto PUBLIC OpenSSL::SSL OpenSSL::Crypto)
