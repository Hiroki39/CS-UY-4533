# This CMakeLists is intended only for macOS and Linux.
cmake_minimum_required(VERSION 3.15)

# Set a project name.
project(HelloOpenGL)

# Use the C++11 standard.
set(CMAKE_CXX_FLAGS "-std=c++11 -g")

# Suppress warnings of the deprecation of glut functions on macOS.
if(APPLE)
   add_definitions(-Wno-deprecated-declarations)
endif()

# Find the packages we need.
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Linux
# If not on macOS, we need glew.
if(UNIX AND NOT APPLE)
   find_package(GLEW REQUIRED)
endif()

# OPENGL_INCLUDE_DIR, GLUT_INCLUDE_DIR, OPENGL_LIBRARIES, and GLUT_LIBRARIES
# are CMake built-in variables defined when the packages are found.
set(INCLUDE_DIRS ${OPENGL_INCLUDE_DIR} ${GLUT_INCLUDE_DIR})
set(LIBRARIES ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})

# If not on macOS, add glew include directory and library path to lists.
if(UNIX AND NOT APPLE)
   list(APPEND INCLUDE_DIRS ${GLEW_INCLUDE_DIRS})
   list(APPEND LIBRARIES ${GLEW_LIBRARIES})
endif()

# Add the list of include paths to be used to search for include files.
include_directories(${INCLUDE_DIRS})

# Search all the .cpp files in the directory where CMakeLists lies and set them to ${SOURCE_FILES}.
# Search all the .h files in the directory where CMakeLists lies and set them to ${INCLUDE_FILES}.
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB INCLUDE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)

# Add the executable Example to be built from the source files.
add_executable(hw01 ${SOURCE_FILES} ${INCLUDE_FILES})

# Link the executable to the libraries.
target_link_libraries(hw01 ${LIBRARIES})
