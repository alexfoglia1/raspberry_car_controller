cmake_minimum_required(VERSION 3.10)
project(raspberry_car_controller CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

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
${CMAKE_SOURCE_DIR}/src/include/data_interface.h
${CMAKE_SOURCE_DIR}/src/include/video_interface.h

${CMAKE_SOURCE_DIR}/src/core/video_renderer.cpp
${CMAKE_SOURCE_DIR}/src/core/joystick.cpp
${CMAKE_SOURCE_DIR}/src/core/widgets.cpp
${CMAKE_SOURCE_DIR}/src/core/utils.cpp
${CMAKE_SOURCE_DIR}/src/core/cbit.cpp
${CMAKE_SOURCE_DIR}/src/core/data_interface.cpp
${CMAKE_SOURCE_DIR}/src/core/video_interface.cpp

${CMAKE_SOURCE_DIR}/src/core/keymap.cpp
${CMAKE_SOURCE_DIR}/src/main/main.cpp
)

target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/include)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/core)
target_include_directories(raspberry_car_controller PRIVATE ${CMAKE_SOURCE_DIR}/src/main)

find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core Gui Widgets Network OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(libssh REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS})
target_link_libraries(raspberry_car_controller ${OpenCV_LIBS} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL Qt5::Network stdc++fs pthread ssh )

install(TARGETS raspberry_car_controller DESTINATION bin)
