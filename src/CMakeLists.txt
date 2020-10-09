set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCE_FILES ./*.cc)

add_executable(vrc_photo_streamer ${SOURCE_FILES})
target_link_libraries(
  vrc_photo_streamer
  ${Boost_LIBRARIES}
  ${GMXX_LIBS}
  ${GstRtspServer_LIBRARIES}
  ${OpenCV_LIBS}
  ${PNG_LIBRARIES}
  Threads::Threads)
