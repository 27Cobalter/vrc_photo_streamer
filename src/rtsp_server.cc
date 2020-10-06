#include <chrono>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>

#include "rtsp_server.h"

namespace vrc_photo_streamer::rtsp {

void rtsp_server::need_data(GstElement* appsrc, guint unused, context* ctx) {
  GstBuffer* buffer;
  guint size;
  GstFlowReturn ret;

  // TODO: このブロックの処理分ける
  cv::Mat image = cv::imread("resources/VRChat_1920x1080_2020-03-10_02-43-00.711.png");
  auto now      = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  cv::putText(image, std::ctime(&now), cv::Point(0, 100), cv::FONT_HERSHEY_SIMPLEX, 2.5,
              cv::Scalar(255, 255, 255), 3);
  size = image.cols * image.rows * image.channels();

  // imageのデータをgstのバッファに書き込む
  buffer = gst_buffer_new_allocate(NULL, size, NULL);
  gst_buffer_fill(buffer, 0, image.data, size);

  GST_BUFFER_PTS(buffer)      = ctx->timestamp;
  GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale_int(1, GST_SECOND, 5);
  ctx->timestamp += GST_BUFFER_DURATION(buffer);

  // auto end = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  // std::cout << "Time: " << std::chrono::milliseconds(end - now).count() << ", " <<
  // ctime(&end)
  //           << std::endl;

  g_signal_emit_by_name(appsrc, "push-buffer", buffer, &ret);
  gst_buffer_unref(buffer);
}

void rtsp_server::media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media,
                                  gpointer user_data) {
  GstElement *element, *appsrc;
  context* ctx;

  element = gst_rtsp_media_get_element(media);

  appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "vrc_photo_streamer");

  // 入力のフォーマット指定
  gst_util_set_object_arg(G_OBJECT(appsrc), "format", "time");
  g_object_set(G_OBJECT(appsrc), "caps",
               gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGR", "width",
                                   G_TYPE_INT, 1920, "height", G_TYPE_INT, 1080, "framerate",
                                   GST_TYPE_FRACTION, 5, 1, NULL),
               NULL);

  ctx            = g_new0(context, 1);
  ctx->timestamp = 0;

  g_object_set_data_full(G_OBJECT(media), "my-extra-data", ctx, (GDestroyNotify)g_free);

  g_signal_connect(appsrc, "need-data", (GCallback)need_data, ctx);
  gst_object_unref(appsrc);
  gst_object_unref(element);
}
void rtsp_server::initialize(int argc, char** argv) {
  GstRTSPMountPoints* mounts;
  gst_init(&argc, &argv);

  this->loop = g_main_loop_new(NULL, FALSE);

  this->server = gst_rtsp_server_new();

  mounts = gst_rtsp_server_get_mount_points(this->server);

  this->factory = gst_rtsp_media_factory_new();
  gst_rtsp_media_factory_set_shared(this->factory, TRUE);
  gst_rtsp_media_factory_set_launch(
      this->factory,
      "( appsrc name=vrc_photo_streamer is-live=true format=GST_FORMAT_TIME ! videoconvert ! "
      "x264enc "
      "bitrate=16384 key-int-max=1 speed-preset=ultrafast tune=zerolatency ! "
      "video/x-h264,profile=baseline ! rtph264pay config-interval=1 name=pay0 pt=98 )");

  g_signal_connect(this->factory, "media-configure", (GCallback)media_configure, NULL);

  gst_rtsp_mount_points_add_factory(mounts, "/test", this->factory);

  g_object_unref(mounts);

  gst_rtsp_server_attach(this->server, NULL);
}
void rtsp_server::run() {
  g_print("stream ready at rtsp://127.0.0.1:8554/test\n");
  g_main_loop_run(this->loop);
}

} // namespace vrc_photo_streamer::rtsp
