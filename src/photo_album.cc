#include "photo_album.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace vrc_photo_streamer::photo {

int photo_album::find_images() {
  if (!filesystem::is_directory(resources_dir_)) {
    throw std::runtime_error(std::string(resources_dir_) + " is not directory.");
  }

  resource_paths_.clear();

  for (const filesystem::directory_entry& x : filesystem::directory_iterator(resources_dir_)) {
    std::string path(x.path());
    if (path.substr(path.length() - 3) == "png") {
      resource_paths_.insert(x.path());
    }
  }

  // 新しい写真から表示していきたいのでrbegin, rendを使う
  // for(auto it = resource_paths_.rbegin();it!= resource_paths_.rend();it++)
  //   std::cout << *it << std::endl;

  return resource_paths_.size();
}

void photo_album::update(page_data format) {
  cv::Mat working  = cv::Mat::zeros(rows_, cols_, CV_8UC3);
  auto resource_it = resource_paths_.rbegin();

  for (int i = 0; i < format.start; i++) resource_it++;
  for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths_.rend();
       i++, resource_it++) {
    cv::Mat image = cv::imread(*resource_it);
    double tiling = 1.0 / format.tiling;
    int tx        = (i % format.tiling) * cols_ / format.tiling;
    int ty        = (i / format.tiling) * rows_ / format.tiling;

    cv::Mat affine = (cv::Mat_<double>(2, 3) << tiling, 0, tx, 0, tiling, ty);
    cv::warpAffine(image, working, affine, working.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
  }

  std::lock_guard<std::mutex> lock(mutex_);

  *output_frame_ = working.clone();
}

std::shared_ptr<cv::Mat> photo_album::get_frame() {
  std::lock_guard<std::mutex> lock(mutex_);
  return output_frame_;
}
} // namespace vrc_photo_streamer::photo