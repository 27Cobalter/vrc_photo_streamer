#ifndef VRC_PHOTO_STREAMER_PHOTO_ALBUM_H
#define VRC_PHOTO_STREAMER_PHOTO_ALBUM_H

#include <filesystem>
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

class photo_album {
public:
  photo_album(int argc, char** argv, int output_cols, int output_rows);
  ~photo_album();
  int find_images();
  void update(page_data format);
  std::shared_ptr<cv::Mat> get_frame_ptr();

private:
  void put_meta_text(cv::Mat& mat, meta_tool::meta_tool& meta);
  std::mutex mutex_;
  static constexpr char resources_dir_[] = "resources";
  std::set<std::filesystem::path> resource_paths_;
  int output_cols_;
  int output_rows_;
  // thread
  const int thread_num_ = 5;
  boost::asio::io_service thread_;
  boost::asio::io_service::work* work_;
  boost::thread_group thread_group_;
  std::atomic<int> thread_queue_ = 0;
  ;

  std::shared_ptr<cv::Mat> output_frame_ = std::make_shared<cv::Mat>();
};
} // namespace vrc_photo_streamer::photo
#endif
