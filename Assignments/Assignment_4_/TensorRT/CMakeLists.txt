cmake_minimum_required(VERSION 3.5.0)
project(TRT_OBJECT_DETECTION VERSION 0.1.0 LANGUAGES C CXX)

# Find packages
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
find_package(CUDA REQUIRED)

# TensorRT search paths
set(TensorRT_SEARCH_PATHS
    ${TensorRT_DIR}          # User-specified TensorRT directory
    /usr                     # Common installation path
    /usr/local               # Alternative installation path
    /usr/lib/x86_64-linux-gnu
    /usr/include/x86_64-linux-gnu
)

# Locate TensorRT include and libraries
find_path(TensorRT_INCLUDE_DIR NvInfer.h PATHS ${TensorRT_SEARCH_PATHS} PATH_SUFFIXES include)
find_library(TensorRT_LIBRARY nvinfer PATHS ${TensorRT_SEARCH_PATHS} PATH_SUFFIXES lib lib64)
find_library(TensorRT_NVONNXPARSER_LIBRARY nvonnxparser PATHS ${TensorRT_SEARCH_PATHS} PATH_SUFFIXES lib lib64)
find_library(TensorRT_NVPARSERS_LIBRARY nvparsers PATHS ${TensorRT_SEARCH_PATHS} PATH_SUFFIXES lib lib64)

# Check TensorRT headers for versioning
if(TensorRT_INCLUDE_DIR AND EXISTS "${TensorRT_INCLUDE_DIR}/NvInfer.h")
    file(STRINGS "${TensorRT_INCLUDE_DIR}/NvInfer.h" TensorRT_MAJOR REGEX "^#define NV_TENSORRT_MAJOR [0-9]+.*$")
    file(STRINGS "${TensorRT_INCLUDE_DIR}/NvInfer.h" TensorRT_MINOR REGEX "^#define NV_TENSORRT_MINOR [0-9]+.*$")
    file(STRINGS "${TensorRT_INCLUDE_DIR}/NvInfer.h" TensorRT_PATCH REGEX "^#define NV_TENSORRT_PATCH [0-9]+.*$")

    string(REGEX REPLACE "^#define NV_TENSORRT_MAJOR ([0-9]+).*$" "\\1" TensorRT_VERSION_MAJOR "${TensorRT_MAJOR}")
    string(REGEX REPLACE "^#define NV_TENSORRT_MINOR ([0-9]+).*$" "\\1" TensorRT_VERSION_MINOR "${TensorRT_MINOR}")
    string(REGEX REPLACE "^#define NV_TENSORRT_PATCH ([0-9]+).*$" "\\1" TensorRT_VERSION_PATCH "${TensorRT_PATCH}")
    set(TensorRT_VERSION_STRING "${TensorRT_VERSION_MAJOR}.${TensorRT_VERSION_MINOR}.${TensorRT_VERSION_PATCH}")
endif()

# Handle errors if TensorRT is not found
if(NOT TensorRT_INCLUDE_DIR OR NOT TensorRT_LIBRARY)
    message(FATAL_ERROR "TensorRT not found! Please specify the TensorRT_DIR.")
endif()

# Include TensorRT library and include directory
include_directories(${TensorRT_INCLUDE_DIR})
set(TensorRT_LIBRARIES ${TensorRT_LIBRARY} ${TensorRT_NVONNXPARSER_LIBRARY} ${TensorRT_NVPARSERS_LIBRARY})

# Find GStreamer
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)

# Include directories
include_directories(
    ${CUDA_INCLUDE_DIRS}
    ${TensorRT_INCLUDE_DIR}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

# Linker directories
link_directories(
    ${CUDA_TOOLKIT_ROOT_DIR}/lib64
    ${GSTREAMER_LIBRARY_DIRS}
    ${GSTREAMER_APP_LIBRARY_DIRS}
)

# Libraries to link
set(LIBS 
    ${CUDA_LIBRARIES}
    ${TensorRT_LIBRARIES}
    ${GSTREAMER_LIBRARIES}
    -lnvinfer 
    -lnvonnxparser 
    -lcudart 
    -lnvinfer_plugin
)

# Add sources and headers
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)

# Create executable
add_executable(TRT_OBJECT_DETECTION ${SOURCES} ${HEADERS})

# Link libraries to the executable
target_link_libraries(TRT_OBJECT_DETECTION ${LIBS} ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} pthread)