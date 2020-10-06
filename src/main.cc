#include <thread>

#include <opencv2/core/mat.hpp>

#include "rtsp_server.h"
#include "photo_album.h"

int main(int argc, char** argv) {
  using namespace vrc_photo_streamer;
  photo::photo_album album;
  album.test();
  album.update();
  cv::Mat frame = album.get_frame();

  rtsp::rtsp_server rtsp_server;
  rtsp_server.initialize(argc, argv, frame.data, frame.total() * frame.channels());

  std::thread rs([&rtsp_server] { rtsp_server.run(); });

  rs.join();
  return 0;
}
