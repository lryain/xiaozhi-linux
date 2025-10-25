cmake_minimum_required(VERSION 3.10)
project(lvgl)

set(CMAKE_C_STANDARD 99)#C99 # lvgl officially support C99 and above
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(lv_100ask_xz_ai)
add_subdirectory(lvgl)
target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})

#add_subdirectory(lv_100ask_xz_ai)

# Find Freetype
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE REQUIRED freetype2)
if(FREETYPE_FOUND)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    link_directories(${FREETYPE_LIBRARY_DIRS})
    # Add FreeType include directories to lvgl target
    target_include_directories(lvgl PUBLIC ${FREETYPE_INCLUDE_DIRS})
endif()

add_executable(main main.c mouse_cursor_icon.c)

# Optional: LibDRM
include(${CMAKE_CURRENT_LIST_DIR}/lvgl/tests/FindLibDRM.cmake)
if(Libdrm_FOUND)
    include_directories(${Libdrm_INCLUDE_DIRS})
endif()

# Optional: SDL2
find_package(SDL2 QUIET)
find_package(SDL2_image QUIET)
if(SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIRS})
endif()
if(SDL2_image_FOUND)
    include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
endif()

# Collect link libraries
set(LINK_LIBS lv_100ask_xz_ai lvgl lvgl::examples lvgl::demos lvgl::thorvg m pthread freetype png)
if(FREETYPE_FOUND)
    list(APPEND LINK_LIBS ${FREETYPE_LIBRARIES})
endif()
if(SDL2_FOUND)
    list(APPEND LINK_LIBS ${SDL2_LIBRARIES})
endif()
if(SDL2_image_FOUND)
    list(APPEND LINK_LIBS ${SDL2_IMAGE_LIBRARIES})
endif()
if(Libdrm_FOUND)
    list(APPEND LINK_LIBS ${Libdrm_LIBRARIES})
endif()

target_link_libraries(main ${LINK_LIBS})
add_custom_target (run COMMAND ${EXECUTABLE_OUTPUT_PATH}/lv_xiaozhiui DEPENDS main)

