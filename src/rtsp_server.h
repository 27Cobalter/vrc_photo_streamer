#ifndef vrc_photo_streamer_rtsp_server_H
#define vrc_photo_streamer_rtsp_server_H

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

namespace vrc_photo_streamer::rtsp {
typedef struct {
  GstClockTime timestamp;
} context;

class rtsp_server {
public:
  static void need_data(GstElement* appsrc, guint unused, context* ctx);
  static void media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                              gpointer user_data);
  void initialize(int argc, char** argv);
  void run();

private:
  GMainLoop* loop;
  GstRTSPServer* server;
  GstRTSPMediaFactory* factory;
};
} // namespace vrc_photo_streamer::rtsp

#endif
