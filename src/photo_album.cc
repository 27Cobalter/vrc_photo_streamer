#include <filesystem>
#include <iostream>
#include <mutex>
#include <cmath>
#include <chrono>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "photo_album.h"

namespace vrc_photo_streamer::photo {
int photo_album::find_images() {
  if (!std::filesystem::is_directory(resources_dir)) {
    throw std::runtime_error(std::string(resources_dir) + " is not directory.");
  }

  resource_paths.clear();
  for (const std::filesystem::directory_entry& x :
       std::filesystem::directory_iterator(resources_dir)) {
    std::string path(x.path());
    if (path.substr(path.length() - 3) == "png") {
      resource_paths.insert(x.path());
    }
  }

  return resource_paths.size();
}

void photo_album::update(page_format format) {
  cv::Mat working  = cv::Mat::zeros(rows, cols, CV_8UC3);
  auto resource_it = resource_paths.begin();
  for (int i = 0; i < format.begin; i++) resource_it++;
  for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths.end();
       i++, resource_it++) {
    cv::Mat image  = cv::imread(*resource_it);
    double tiling  = 1.0 / format.tiling;
    int tx         = (i % format.tiling) * cols / format.tiling;
    int ty         = (i / format.tiling) * rows / format.tiling;
    cv::Mat affine = (cv::Mat_<double>(2, 3) << tiling, 0, tx, 0, tiling, ty);
    cv::warpAffine(image, working, affine, working.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
  }

  std::lock_guard<std::mutex> lock(mutex_);

  output_frame = working.clone();
}
cv::Mat& photo_album::get_frame() {
  std::lock_guard<std::mutex> lock(mutex_);
  return output_frame;
}
} // namespace vrc_photo_streamer::photo
