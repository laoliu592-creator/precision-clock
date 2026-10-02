cmake_minimum_required(VERSION 3.30.5)
project(precision_clock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(clock SHARED
    native/jni/native_bridge.cpp
    native/clock/clock_engine.cpp
    native/ntp/ntp_client.cpp
    native/ntp/ntp_packet.cpp
    native/socket/udp_socket.cpp
    native/filter/ntp_filter.cpp
)

target_include_directories(clock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/native
)

target_compile_options(clock PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wshadow
    -Wpedantic
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(clock
    ${log-lib}
    ${android-lib}
)
