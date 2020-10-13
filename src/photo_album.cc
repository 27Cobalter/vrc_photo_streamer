#include "photo_album.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

#include <Magick++.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "vrc_meta_tool.h"

namespace vrc_photo_streamer::photo {

photo_album::photo_album(int argc, char** argv, int output_cols, int output_rows)
    : output_cols_(output_cols), output_rows_(output_rows) {
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
  std::shared_ptr<cv::Mat> working =
      std::make_shared<cv::Mat>(cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3));
  auto resource_it   = resource_paths_.rbegin();
  auto interpolation = cv::INTER_AREA;

  for (int i = 0; i < format.start; i++) resource_it++;
  auto func = [&](int i, auto resource_it) {
    std::unique_ptr<cv::Mat> image;
    image        = std::make_unique<cv::Mat>(cv::imread(*resource_it));
    double scale = (static_cast<double>(output_rows_) / image->rows) / format.tiling;
    int tx       = (i % format.tiling) * output_cols_ / format.tiling;
    int ty       = (i / format.tiling) * output_rows_ / format.tiling;

    if (format.tiling == 1) {
      interpolation = cv::INTER_LINEAR;
      try {
        meta_tool::meta_tool meta;
        meta.read(*resource_it);
        if (meta.date().has_value() || meta.photographer().has_value() ||
            meta.world().has_value() || (meta.users().size() != 0)) {
          scale *= 0.8;
          constexpr int text_rows = 540;
          constexpr int text_cols = 960;
          std::shared_ptr<cv::Mat> text =
              std::make_shared<cv::Mat>(cv::Mat::zeros(text_rows, text_cols, CV_8UC3));
          put_meta_text(text, meta);
          cv::resize(*text, *working, working->size(), cv::INTER_NEAREST);
        }
      } catch (std::runtime_error e) {
        // 自分で投げたものであればpngじゃないファイルを開いたとき
      }
    }

    cv::Mat affine = (cv::Mat_<double>(2, 3) << scale, 0, tx, 0, scale, ty);
    cv::warpAffine(*image, *working, affine, working->size(), interpolation,
                   cv::BORDER_TRANSPARENT);
    image.reset();
  };

  std::vector<std::thread> thread_group;
  for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths_.rend();
       i++, resource_it++) {
    thread_group.push_back(std::thread(func, i, resource_it));
  }
  for (auto thread_it = thread_group.begin(); thread_it != thread_group.end(); thread_it++) {
    thread_it->join();
  }

  std::lock_guard<std::mutex> lock(mutex_);

  *output_frame_ = working->clone();
}

std::shared_ptr<cv::Mat> photo_album::get_frame_ptr() {
  std::lock_guard<std::mutex> lock(mutex_);
  return output_frame_;
}

// opencvでは日本語描画できないのでImageMagick使う
void photo_album::put_meta_text(std::shared_ptr<cv::Mat> mat, meta_tool::meta_tool& meta) {
  std::unique_ptr<Magick::Image> image = std::make_unique<Magick::Image>(
      Magick::Geometry(mat->cols, mat->rows), Magick::ColorMono(false));

  const int point_size = mat->rows * 0.08;
  const cv::Point date_pos(0, mat->rows * 0.8 + mat->rows * 0.09);
  const cv::Point world_pos(0, mat->rows * 0.8 + mat->rows * 0.18);
  const int user_point_size = point_size / 2;
  cv::Point user_pos(mat->cols * 0.8 + 10, user_point_size);

  // const cv::Scalar color(255, 255, 0);

  std::vector<Magick::Drawable> draw_list;

  draw_list.push_back(Magick::DrawableFont("Noto-Sans-CJK-JP"));
  draw_list.push_back(Magick::DrawableFillColor(Magick::ColorMono(false)));
  draw_list.push_back(Magick::DrawablePointSize(point_size));

  if (meta.date().has_value()) {
    draw_list.push_back(Magick::DrawablePointSize(point_size));
    draw_list.push_back(
        Magick::DrawableText(date_pos.x, date_pos.y, meta.readable_date().value(), "utf-8"));
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
  image->draw(draw_list);
  image->write(0, 0, mat->cols, mat->rows, "BGR", Magick::CharPixel, mat->data);
  image.reset();
}
} // namespace vrc_photo_streamer::photo
