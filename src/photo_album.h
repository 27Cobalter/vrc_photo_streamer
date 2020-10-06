#ifndef VRC_PHOTO_STREAMER_PHOTO_ALBUM_H
#define VRC_PHOTO_STREAMER_PHOTO_ALBUM_H

#include <filesystem>
#include <set>
#include <mutex>
#include <optional>

#include <opencv2/core/mat.hpp>

namespace vrc_photo_streamer::photo {
class photo_album {
public:
  void test();
  void update();
  cv::Mat& get_frame();

private:
  std::mutex mutex_;
  static constexpr char resources_dir[] = "resources";

  int page = 0;
  std::optional<int> select = std::nullopt;
  cv::Mat output_frame;
};
} // namespace vrc_photo_streamer::photo
#endif
