cmake_minimum_required(VERSION 3.5.0)
project(RTSP_Stream VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14) # Setting C++ 14 standard

# Set the GStreamer installation directory
set(GSTREAMER_ROOT "E:/gstreamer/1.0/msvc_x86_64") # Change this path as necessary

# Automatically find all include directories under the GStreamer include path
file(GLOB_RECURSE GSTREAMER_INCLUDE_DIRS
    "${GSTREAMER_ROOT}/include/*"
)

# Include the GStreamer header files
include_directories(${GSTREAMER_INCLUDE_DIRS})

# Specify the library directories for GStreamer
link_directories("${GSTREAMER_ROOT}/lib")

# Building target executable
add_executable(${PROJECT_NAME} main.cpp)

# Linking GStreamer libraries with target executable
target_link_libraries(${PROJECT_NAME}
    gstreamer-1.0
    gstreamer-base-1.0
    gstreamer-app-1.0
    gobject-2.0
    glib-2.0
)
