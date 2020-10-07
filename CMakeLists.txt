cmake_minimum_required(VERSION 3.18)

project(vrc_photo_streamer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -O2 -pipe -fstack-protector-strong -fno-plt -fno-omit-frame-pointer"
)

if(CMAKE_GENEROTOR STREQUAL "Ninja")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

message(STATUS "Detecting CPU architecture " ${CMAKE_SYSTEM_PROCESSOR})
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GstRtspServer gstreamer-rtsp-server-1.0 REQUIRED)
include_directories(${GstRtspServer_INCLUDE_DIRS})

file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
add_subdirectory(src)
