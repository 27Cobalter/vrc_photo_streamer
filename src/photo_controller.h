#ifndef VRC_PHOTO_STREAMER_PHOTO_CONTROLLER_H
#define VRC_PHOTO_STREAMER_PHOTO_CONTROLLER_H

#include <optional>

#include "photo_album.h"

namespace vrc_photo_streamer::controller {

class photo_controller {
public:
  photo_controller(int argc, char** argv, int output_cols, int output_rows);
  int find_images();
  std::shared_ptr<cv::Mat> get_frame_ptr();
  void next();
  void prev();
  void head();
  void select(std::optional<int> num);

private:
  void update(photo::page_data page);
  int tile2(photo::page_data page);
  photo::page_data current_page_;
  photo::page_data tiling_page_;
  std::unique_ptr<photo::photo_album> photo_album_;
  size_t end_;
};
} // namespace vrc_photo_streamer::controller
#endif
