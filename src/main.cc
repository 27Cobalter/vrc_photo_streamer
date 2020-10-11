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
  constexpr int stream_cols = 1920;
  constexpr int stream_rows = 1080;

  std::shared_ptr<controller::photo_controller> controller =
      std::make_shared<controller::photo_controller>(argc, argv, stream_cols, stream_rows);

  http::http_server http_server(controller);
  std::thread hs([&http_server] { http_server.run(); });

  rtsp::rtsp_server rtsp_server;
  std::shared_ptr<cv::Mat> frame = controller->get_frame_ptr();
  rtsp_server.initialize(argc, argv, frame);

  std::thread rs([&rtsp_server] { rtsp_server.run(); });

  hs.join();
  rs.join();

  return 0;
}
