#ifndef VRC_PHOTO_STREAMER_RTSP_SERVER_H
#define VRC_PHOTO_STREAMER_RTSP_SERVER_H

#include <memory>

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

namespace vrc_photo_streamer::rtsp {

namespace chrono = std::chrono;
typedef struct {
  GstClockTime timestamp;
} context;

class rtsp_server {
public:
  static void need_data(GstElement* appsrc, guint unused, context* ctx);
  static void media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                              gpointer user_data);
  void initialize(int argc, char** argv, std::shared_ptr<cv::Mat> frame);
  void run();

private:
  static std::shared_ptr<cv::Mat> frame_;
  static guint size_;
  static int output_cols_;
  static int output_rows_;
  GMainLoop* loop_;
  GstRTSPServer* server_;
  GstRTSPMediaFactory* factory_;
};
} // namespace vrc_photo_streamer::rtsp
#endif
