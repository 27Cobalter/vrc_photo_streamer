#include <iostream>
#include <memory>
#include <thread>

#include <opencv2/core/mat.hpp>

#include "http_server.h"
#include "photo_album.h"
#include "photo_controller.h"
#include "rtsp_server.h"

int main(int argc, char** argv) {
  using namespace vrc_photo_streamer;

  http::http_server http_server;
  std::thread hs([&http_server] { http_server.run(); });

  rtsp::rtsp_server rtsp_server;
  std::shared_ptr<cv::Mat> frame = controller::photo_controller::get_frame();
  rtsp_server.initialize(argc, argv, frame, frame->total() * frame->channels());

  std::thread rs([&rtsp_server] { rtsp_server.run(); });

  hs.join();
  rs.join();

  return 0;
}
