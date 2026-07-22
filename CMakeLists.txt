cmake_minimum_required(VERSION 3.16)
project(VirtualAutomotiveECUSim VERSION 0.1 LANGUAGES C CXX)

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(common/can)
add_subdirectory(common/crypto)
add_subdirectory(common/hsm)
add_subdirectory(common/scheduler)
add_subdirectory(common/uds)
add_subdirectory(ecus/hsm_service)
add_subdirectory(ecus/sensor)
add_subdirectory(ecus/radar_sensor)
add_subdirectory(ecus/bms)
add_subdirectory(ecus/tpms)
add_subdirectory(ecus/gateway)
add_subdirectory(tests)
