cmake_minimum_required(VERSION 3.10)
project(sound_app)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED alsa)
pkg_check_modules(OPUS REQUIRED opus)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)
find_package(Threads REQUIRED)

# Add executable
add_executable(sound_app
    sound_app.cpp
    aplay.cpp
    record.cpp
    opus.cpp
    ipc_udp.cpp
)

# Include directories
target_include_directories(sound_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ALSA_INCLUDE_DIRS}
    ${OPUS_INCLUDE_DIRS}
    ${SPEEXDSP_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(sound_app
    ${ALSA_LIBRARIES}
    ${OPUS_LIBRARIES}
    ${SPEEXDSP_LIBRARIES}
    Threads::Threads
)