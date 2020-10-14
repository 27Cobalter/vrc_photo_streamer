#ifndef VRC_PHOTO_STREAMER_PHOTO_ALBUM_H
#define VRC_PHOTO_STREAMER_PHOTO_ALBUM_H

#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <set>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/thread_pool.hpp>
#include <opencv2/core/mat.hpp>

#include "vrc_meta_tool.h"

namespace vrc_photo_streamer::photo {

namespace filesystem = std::filesystem;

typedef struct {
  int start;
  int tiling;
} page_data;

typedef struct {
  int page_start;
  boost::thread_group threads;
  std::shared_ptr<cv::Mat> image;
} image_buffer;

class photo_album {
public:
  photo_album(int argc, char** argv, int output_cols, int output_rows);
  int find_images();
  void update(page_data format);
  std::shared_ptr<cv::Mat> get_frame_ptr();

private:
  void put_meta_text(std::shared_ptr<cv::Mat> mat, meta_tool::meta_tool& meta);
  int calc_buffer_pos(int delta);
  std::mutex mutex_;
  static constexpr char resources_dir_[] = "resources";
  std::set<std::filesystem::path> resource_paths_;
  int output_cols_;
  int output_rows_;
  std::shared_ptr<cv::Mat> output_frame_ = std::make_shared<cv::Mat>();
  // 現在のページ+前後1ページ
  static constexpr int buffer_size_ = 3;
  image_buffer image_buffer_[buffer_size_];
  int buffer_head_ = 0;
};
} // namespace vrc_photo_streamer::photo
#endif
