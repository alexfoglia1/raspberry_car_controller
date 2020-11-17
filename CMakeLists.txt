cmake_minimum_required(VERSION 3.1.0)
project(raspberry_car_controller)

add_executable(
raspberry_car_controller

${CMAKE_SOURCE_DIR}/src/include/defs.h
${CMAKE_SOURCE_DIR}/src/include/data_transceiver.h
${CMAKE_SOURCE_DIR}/src/include/data_server.h
${CMAKE_SOURCE_DIR}/src/include/video_receiver.h
${CMAKE_SOURCE_DIR}/src/include/video_server.h
${CMAKE_SOURCE_DIR}/src/include/video_renderer.h
${CMAKE_SOURCE_DIR}/src/include/logger.h

${CMAKE_SOURCE_DIR}/src/core/data_transceiver.c
${CMAKE_SOURCE_DIR}/src/core/video_receiver.c
${CMAKE_SOURCE_DIR}/src/core/video_renderer.c
${CMAKE_SOURCE_DIR}/src/core/logger.c

${CMAKE_SOURCE_DIR}/src/main/data_server.c
${CMAKE_SOURCE_DIR}/src/main/video_server.c
${CMAKE_SOURCE_DIR}/src/main/main.c
)

target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/main)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS})
target_link_libraries(raspberry_car_controller ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
target_link_libraries(raspberry_car_controller "-pthread")
