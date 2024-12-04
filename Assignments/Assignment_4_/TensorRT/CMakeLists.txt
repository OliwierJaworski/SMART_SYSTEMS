cmake_minimum_required(VERSION 3.5.0)
project(TensorRT_Project VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -DDEBUG")

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

# Find OpenCV
find_package(OpenCV REQUIRED)

# CUDA settings
find_package(CUDA REQUIRED)
set(CUDA_TOOLKIT_ROOT_DIR /usr/local/cuda)

# TensorRT settings
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package(TensorRT REQUIRED)

# Include directories
include_directories(
    ${CUDA_INCLUDE_DIRS}
    ${TensorRT_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${OpenCV_INCLUDE_DIRS}  # Include OpenCV headers
)

# Linker directories
link_directories(
    ${CUDA_TOOLKIT_ROOT_DIR}/lib64
    ${TensorRT_LIBRARY_DIRS}
    ${GStreamer_LIBRARY_DIRS}
    ${GSTREAMER_APP_LIBRARY_DIRS}
)

# Libraries to link
set(LIBS 
    ${CUDA_LIBRARIES}
    ${TensorRT_LIBRARIES}
    ${GStreamer_LIBRARIES}
    ${OpenCV_LIBRARIES}  # Link OpenCV
    -lnvinfer 
    -lnvonnxparser 
    -lcudart 
    -lnvinfer_plugin
)

# Source files
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)

# Add executable
add_executable(TensorRT_Project main.cpp ${SOURCES} ${HEADERS})

# Link libraries to the executable
target_link_libraries(TensorRT_Project ${LIBS} ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} pthread)
