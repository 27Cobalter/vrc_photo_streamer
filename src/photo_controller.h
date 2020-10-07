#ifndef VRC_PHOTO_STREAMER_PHOTO_CONTROLLER_H
#define VRC_PHOTO_STREAMER_PHOTO_CONTROLLER_H

#include <optional>

#include "photo_album.h"

namespace vrc_photo_streamer::controller {

class photo_controller {
public:
  static int find_images();
  static cv::Mat& get_frame();
  static void next();
  static void prev();
  static void select(std::optional<int> num);

private:
  static void update(photo::page_data page);
  static int tile2(photo::page_data page);
  static photo::page_data current_page_;
  static photo::page_data tiling_page_;
  static photo::photo_album photo_album_;
  static size_t end;
};
} // namespace vrc_photo_streamer::controller
#endif
