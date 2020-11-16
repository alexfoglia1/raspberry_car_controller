cmake_minimum_required(VERSION 2.6)
project(raspberry_car_controller)

add_executable(
raspberry_car_controller

${CMAKE_SOURCE_DIR}/src/include/defs.h
${CMAKE_SOURCE_DIR}/src/include/data_transceiver.h
${CMAKE_SOURCE_DIR}/src/include/video_receiver.h
${CMAKE_SOURCE_DIR}/src/include/video_renderer.h

${CMAKE_SOURCE_DIR}/src/core/data_transceiver.c
${CMAKE_SOURCE_DIR}/src/core/video_receiver.c
${CMAKE_SOURCE_DIR}/src/core/video_renderer.c

${CMAKE_SOURCE_DIR}/src/main/main.c
)

target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/main)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS})
target_link_libraries(raspberry_car_controller ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
