#ifndef VRC_PHOTO_STREAMER_PHOTO_ALBUM_H
#define VRC_PHOTO_STREAMER_PHOTO_ALBUM_H

#include <filesystem>
#include <set>
#include <mutex>
#include <optional>

#include <opencv2/core/mat.hpp>

namespace vrc_photo_streamer::photo {
typedef struct {
  int begin;
  int tiling;
} page_format;

class photo_album {
public:
  void update(page_format format);
  int find_images();
  cv::Mat& get_frame();

private:
  std::mutex mutex_;
  static constexpr char resources_dir[] = "resources";
  std::set<std::filesystem::path> resource_paths;
  int cols = 1920;
  int rows = 1080;

  cv::Mat output_frame;
};
} // namespace vrc_photo_streamer::photo
#endif
