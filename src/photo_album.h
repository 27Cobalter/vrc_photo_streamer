#ifndef VRC_PHOTO_STREAMER_PHOTO_ALBUM_H
#define VRC_PHOTO_STREAMER_PHOTO_ALBUM_H

#include <filesystem>
#include <mutex>
#include <set>

#include <opencv2/core/mat.hpp>

namespace vrc_photo_streamer::photo {

namespace filesystem = std::filesystem;

typedef struct {
  int start;
  int tiling;
} page_data;

class photo_album {
public:
  int find_images();
  void update(page_data format);
  cv::Mat& get_frame();

private:
  std::mutex mutex_;
  static constexpr char resources_dir_[] = "resources";
  std::set<std::filesystem::path> resource_paths_;
  int cols_ = 1920;
  int rows_ = 1080;

  cv::Mat output_frame_;
};
} // namespace vrc_photo_streamer::photo
#endif
