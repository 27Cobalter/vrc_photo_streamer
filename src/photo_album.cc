#include <filesystem>
#include <iostream>
#include <mutex>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "photo_album.h"

namespace vrc_photo_streamer::photo {
void photo_album::test() {
  if (!std::filesystem::is_directory(resources_dir)) {
    std::cout << resources_dir << " is not directory." << std::endl;
    return;
  }
  std::set<std::filesystem::path> resource_paths;

  for (const std::filesystem::directory_entry& x :
       std::filesystem::directory_iterator(resources_dir)) {
    resource_paths.insert(x.path());
  }
}
void photo_album::update() {
  std::set<std::filesystem::path> resource_paths;
  for (const std::filesystem::directory_entry& x :
       std::filesystem::directory_iterator(resources_dir)) {
    resource_paths.insert(x.path());
  }
  cv::Mat image = cv::imread(*resource_paths.begin());
  std::lock_guard<std::mutex> lock(mutex_);

  output_frame = image.clone();
}
cv::Mat& photo_album::get_frame() {
  std::lock_guard<std::mutex> lock(mutex_);
  return output_frame;
}
} // namespace vrc_photo_streamer::photo
