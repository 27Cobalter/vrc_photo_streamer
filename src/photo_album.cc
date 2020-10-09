#include "photo_album.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <memory>

#include <Magick++.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "vrc_meta_tool.h"

namespace vrc_photo_streamer::photo {

photo_album::photo_album(int argc, char** argv) {
  Magick::InitializeMagick(*argv);
}

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

    if (format.tiling == 1) {
      try {
        meta_tool::meta_tool meta;
        meta.read(*resource_it);
        if (meta.date().has_value() || meta.photographer().has_value() ||
            meta.world().has_value() || (meta.users().size() != 0)) {
          tiling = 0.8;
          put_meta_text(working, meta);
        }
      } catch (std::runtime_error e) {
      }
    }

    cv::Mat affine = (cv::Mat_<double>(2, 3) << tiling, 0, tx, 0, tiling, ty);
    cv::warpAffine(image, working, affine, working.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
  }

  std::lock_guard<std::mutex> lock(mutex_);

  *output_frame_ = working.clone();
}

std::shared_ptr<cv::Mat> photo_album::get_frame_ptr() {
  std::lock_guard<std::mutex> lock(mutex_);
  return output_frame_;
}

// opencvでは日本語描画できないのでImageMagick使う
void photo_album::put_meta_text(cv::Mat& mat, meta_tool::meta_tool& meta) {
  // Magick::Image image(1920, 1080, "BGR", Magick::CharPixel, mat.data);
  Magick::Image image(Magick::Geometry(cols_, rows_), Magick::Color(0, 0, 0));

  const int point_size = rows_ * 0.08;
  const cv::Point date_pos(0, rows_ * 0.8 + rows_ * 0.09);
  const cv::Point world_pos(0, rows_ * 0.8 + rows_ * 0.18);
  const int user_point_size = point_size / 2;
  cv::Point user_pos(cols_ * 0.8 + 10, user_point_size);

  // const cv::Scalar color(255, 255, 0);

  std::vector<Magick::Drawable> draw_list;

  draw_list.push_back(Magick::DrawableFont("Noto-Sans-CJK-JP"));
  draw_list.push_back(Magick::DrawableFillColor("white"));
  draw_list.push_back(Magick::DrawablePointSize(point_size));

  if (meta.date().has_value()) {
    draw_list.push_back(Magick::DrawablePointSize(point_size));
    draw_list.push_back(
        Magick::DrawableText(date_pos.x, date_pos.y, meta.date().value(), "utf-8"));
  }
  if (meta.world().has_value()) {
    draw_list.push_back(Magick::DrawablePointSize(point_size));
    draw_list.push_back(Magick::DrawableText(world_pos.x, world_pos.y, meta.world().value()));
  }
  int i = 0;
  for (auto [user_name, screen_name] : meta.users()) {
    draw_list.push_back(Magick::DrawablePointSize(user_point_size));
    draw_list.push_back(
        Magick::DrawableText(user_pos.x, user_pos.y + i * user_point_size, user_name));
    i++;
  }
  image.draw(draw_list);
  image.write(0, 0, cols_, rows_, "BGR", Magick::CharPixel, mat.data);
}
} // namespace vrc_photo_streamer::photo
