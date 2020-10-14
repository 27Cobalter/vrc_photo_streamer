#include "photo_album.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
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
  find_images();
  image_buffer_[buffer_head_].page_start = std::numeric_limits<int>::max();
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
  auto resource_it = resource_paths_.rbegin();

  for (int i = 0; i < format.start; i++) resource_it++;
  auto func = [this](int i, auto resource_it, std::shared_ptr<cv::Mat> working,
                     page_data format, int ms = 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    auto interpolation = cv::INTER_AREA;
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

  if (format.tiling != 1) {
    // 条件式長いので分割
    // 0から末尾方向に行くとき以外で更新する値が大きいとき
    auto conditional1 = [&]() {
      return format.start >= image_buffer_[buffer_head_].page_start &&
             !(format.start != static_cast<int>(std::pow(format.tiling, 2)) &&
               image_buffer_[buffer_head_].page_start == 0);
    };
    // 末尾方向から0に行くとき
    auto conditional2 = [&]() {
      return (image_buffer_[buffer_head_].page_start + std::pow(format.tiling, 2) >
                  resource_paths_.size() &&
              format.start == 0);
    };
    if (image_buffer_[buffer_head_].page_start == format.start) {
      // 1枚選択から戻したとき
    } else if (conditional1() || conditional2()) {
      buffer_head_          = calc_buffer_pos(+1);
      page_data next_format = {format.start + static_cast<int>(std::pow(format.tiling, 2)),
                               format.tiling};
      if (next_format.start >= resource_paths_.size()) next_format.start = 0;
      auto resource_it = std::make_shared<std::set<std::filesystem::path>::reverse_iterator>(
          resource_paths_.rbegin());
      for (int i = 0; i < next_format.start; i++) (*resource_it)++;
      int next_head_                       = calc_buffer_pos(+1);
      image_buffer_[next_head_].page_start = next_format.start;
      image_buffer_[next_head_].image =
          std::make_shared<cv::Mat>(cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3));
      boost::thread_group thread_group;
      for (int i = 0; i < std::pow(format.tiling, 2) && *resource_it != resource_paths_.rend();
           i++, (*resource_it)++) {
        image_buffer_[next_head_].threads.create_thread(std::bind<void>(
            func, i, *resource_it, image_buffer_[next_head_].image, next_format, 1500));
      }
    } else {
      buffer_head_          = calc_buffer_pos(-1);
      page_data next_format = {format.start - static_cast<int>(std::pow(format.tiling, 2)),
                               format.tiling};
      if (next_format.start < 0)
        next_format.start =
            std::trunc((resource_paths_.size() - 1) / std::pow(next_format.tiling, 2)) *
            std::pow(next_format.tiling, 2);
      auto resource_it = std::make_shared<std::set<std::filesystem::path>::reverse_iterator>(
          resource_paths_.rbegin());
      for (int i = 0; i < next_format.start; i++) (*resource_it)++;
      int next_head_                       = calc_buffer_pos(-1);
      image_buffer_[next_head_].page_start = next_format.start;
      image_buffer_[next_head_].image =
          std::make_shared<cv::Mat>(cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3));
      boost::thread_group thread_group;
      for (int i = 0; i < std::pow(format.tiling, 2) && *resource_it != resource_paths_.rend();
           i++, (*resource_it)++) {
        image_buffer_[next_head_].threads.create_thread(std::bind<void>(
            func, i, *resource_it, image_buffer_[next_head_].image, next_format, 1500));
      }
    }
    if (image_buffer_[buffer_head_].page_start != format.start) {
      std::cout << "buffer unhit" << std::endl;
      image_buffer_[buffer_head_].page_start = format.start;
      image_buffer_[buffer_head_].image =
          std::make_shared<cv::Mat>(cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3));
      boost::thread_group thread_group;
      for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths_.rend();
           i++, resource_it++) {
        image_buffer_[buffer_head_].threads.create_thread(
            std::bind<void>(func, i, resource_it, image_buffer_[buffer_head_].image, format));
      }
    } else {
      std::cout << "buffer hit" << std::endl;
    }
    image_buffer_[buffer_head_].threads.join_all();

    std::lock_guard<std::mutex> lock(mutex_);

    *output_frame_ = image_buffer_[buffer_head_].image->clone();
  } else {
    auto image = std::make_shared<cv::Mat>(cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3));
    std::thread thread(std::bind<void>(func, 0, resource_it, image, format));
    thread.join();

    std::lock_guard<std::mutex> lock(mutex_);

    *output_frame_ = image->clone();
  }
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
  draw_list.push_back(Magick::DrawableFillColor(Magick::ColorMono(true)));
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

int photo_album::calc_buffer_pos(int delta) {
  int tmp = (buffer_head_ + delta) % buffer_size_;
  return tmp < 0 ? tmp + buffer_size_ : tmp;
}
} // namespace vrc_photo_streamer::photo
