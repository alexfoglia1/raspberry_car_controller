cmake_minimum_required(VERSION 3.10)
project(raspberry_car_controller CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Cmake prefix path = ${CMAKE_PREFIX_PATH}")

add_executable(
raspberry_car_controller

${CMAKE_SOURCE_DIR}/src/include/defs.h
${CMAKE_SOURCE_DIR}/src/include/video_renderer.h
${CMAKE_SOURCE_DIR}/src/include/keymap.h
${CMAKE_SOURCE_DIR}/src/include/joystick.h
${CMAKE_SOURCE_DIR}/src/include/widgets.h
${CMAKE_SOURCE_DIR}/src/include/utils.h
${CMAKE_SOURCE_DIR}/src/include/cbit.h

${CMAKE_SOURCE_DIR}/src/core/video_renderer.cpp
${CMAKE_SOURCE_DIR}/src/core/joystick.cpp
${CMAKE_SOURCE_DIR}/src/core/widgets.cpp
${CMAKE_SOURCE_DIR}/src/core/utils.cpp
${CMAKE_SOURCE_DIR}/src/core/cbit.cpp

${CMAKE_SOURCE_DIR}/src/core/keymap.cpp
${CMAKE_SOURCE_DIR}/src/main/main.cpp
)

target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/main)

find_package(Threads REQUIRED)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(raspberry_car_controller ${OpenCV_LIBS} stdc++fs xdo pthread)

install(TARGETS raspberry_car_controller DESTINATION bin)
