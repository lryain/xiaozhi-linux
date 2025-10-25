cmake_minimum_required(VERSION 3.10)
project(control_center)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Add executable
add_executable(control_center
    control_center.cpp
    websocket_client.cpp
    ipc_udp.cpp
    http.cpp
    uuid.cpp
)

# Include directories
target_include_directories(control_center PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link libraries
target_link_libraries(control_center
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    CURL::libcurl
)