add_library(common_uds STATIC
    UdsProtocol.cpp
    UdsDispatcher.cpp
    SessionManager.cpp
    SecurityManager.cpp
    DidManager.cpp
    DtcManager.cpp
)
target_include_directories(common_uds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
