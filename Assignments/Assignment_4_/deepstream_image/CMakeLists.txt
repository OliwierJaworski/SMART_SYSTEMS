cmake_minimum_required(VERSION 3.5.0)
set(CMAKE_CUDA_COMPILER /usr/local/cuda/bin/nvcc)
project(image_DS VERSION 0.1.0 LANGUAGES CXX CUDA)

set(TENSORRT_PATH "/usr")

# Allow overriding TensorRT and OpenCV paths via command line
# e.g., cmake -DTENSORRT_PATH="path/to/TensorRT" -DOpenCV_DIR="path/to/OpenCV" ..
option(TENSORRT_PATH_OPTION "Path to TensorRT installation" ${TENSORRT_PATH})
set(TENSORRT_PATH ${TENSORRT_PATH_OPTION} CACHE PATH "Path to TensorRT installation")

find_package(OpenCV REQUIRED)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please install OpenCV or set OpenCV_DIR.")
endif()

find_package(CUDA REQUIRED)
if(NOT CUDA_FOUND)
    message(FATAL_ERROR "CUDA not found. Please install the CUDA Toolkit.")
endif()

find_package(PkgConfig) 
pkg_search_module(GLIB REQUIRED glib-2.0) 
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(GST_RTSP REQUIRED gstreamer-rtsp-server-1.0)  # Add RTSP server package

include_directories(${TENSORRT_PATH}/include)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_compile_definitions(${PROJECT_NAME} PRIVATE API_EXPORTS)

# Specify include directories (modern CMake approach)
target_include_directories(${PROJECT_NAME} PRIVATE
    src/
    /opt/nvidia/deepstream/deepstream/sources/includes
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${TENSORRT_PATH}/include
    ${GLIB_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GST_APP_INCLUDE_DIRS}
    ${GST_VIDEO_INCLUDE_DIRS}
    ${GST_RTSP_INCLUDE_DIRS}  # Include RTSP headers
)

# Link TensorRT libraries
# Specify full paths to TensorRT libraries to avoid relying on link_directories
set(TENSORRT_LIBS
    nvinfer
    nvonnxparser
    nvparsers
    nvinfer_plugin
)

# Link libraries to the target
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    ${TENSORRT_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GST_APP_LIBRARIES}
    ${GST_VIDEO_LIBRARIES}
    ${GST_RTSP_LIBRARIES}  # Link against RTSP libraries
)

# Enable separable compilation for CUDA (optional but recommended)
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)