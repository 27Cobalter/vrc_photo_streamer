#ifndef VRC_PHOTO_STREAMER_RTSP_SERVER_H
#define VRC_PHOTO_STREAMER_RTSP_SERVER_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <opencv2/core/hal/interface.h>

namespace vrc_photo_streamer::rtsp {
typedef struct {
  GstClockTime timestamp;
} context;

class rtsp_server {
public:
  static void need_data(GstElement* appsrc, guint unused, context* ctx);
  static void media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                              gpointer user_data);
  void initialize(int argc, char** argv, uchar* frame, guint size);
  void run();

private:
  static uchar* frame_;
  static guint size_;
  GMainLoop* loop;
  GstRTSPServer* server;
  GstRTSPMediaFactory* factory;
};
} // namespace vrc_photo_streamer::rtsp

#endif
