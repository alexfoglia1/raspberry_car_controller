cmake_minimum_required(VERSION 3.14)
project(raspberry_car_controller CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
raspberry_car_controller

${CMAKE_SOURCE_DIR}/src/include/defs.h
${CMAKE_SOURCE_DIR}/src/include/data_transceiver.h
${CMAKE_SOURCE_DIR}/src/include/data_server.h
${CMAKE_SOURCE_DIR}/src/include/video_receiver.h
${CMAKE_SOURCE_DIR}/src/include/video_server.h
${CMAKE_SOURCE_DIR}/src/include/video_renderer.h
${CMAKE_SOURCE_DIR}/src/include/logger.h
${CMAKE_SOURCE_DIR}/src/include/keymap.h

${CMAKE_SOURCE_DIR}/src/core/data_transceiver.cpp
${CMAKE_SOURCE_DIR}/src/core/video_receiver.cpp
${CMAKE_SOURCE_DIR}/src/core/video_renderer.cpp
${CMAKE_SOURCE_DIR}/src/core/logger.cpp

${CMAKE_SOURCE_DIR}/src/main/data_server.cpp
${CMAKE_SOURCE_DIR}/src/main/video_server.cpp
${CMAKE_SOURCE_DIR}/src/core/keymap.cpp
${CMAKE_SOURCE_DIR}/src/main/main.cpp
)

target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/main)

find_package(Threads REQUIRED)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
target_link_libraries(raspberry_car_controller ${OpenCV_LIBS})
