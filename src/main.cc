#include "rtsp_server.h"

int main(int argc, char** argv) {
  using namespace vrc_photo_streamer;
  rtsp::rtsp_server server;
  server.initialize(argc, argv);
  server.run();
}
