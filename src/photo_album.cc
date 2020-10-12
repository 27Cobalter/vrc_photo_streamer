#include "photo_album.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <memory>
#include <vector>

#include <Magick++.h>
#include <boost/bind.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "vrc_meta_tool.h"

namespace vrc_photo_streamer::photo {

photo_album::photo_album(int argc, char** argv, int output_cols, int output_rows)
    : output_cols_(output_cols), output_rows_(output_rows) {
  Magick::InitializeMagick(*argv);
  work_ = new boost::asio::io_service::work(thread_);
  for (int i = 0; i < 5; i++)
    thread_group_.create_thread(boost::bind(&boost::asio::io_service::run, &thread_));
}

photo_album::~photo_album() {
  delete (work_);
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
  cv::Mat working  = cv::Mat::zeros(output_rows_, output_cols_, CV_8UC3);
  auto resource_it = resource_paths_.rbegin();

  for (int i = 0; i < format.start; i++) resource_it++;
  auto func = [&](int i, auto resource_it) {
    std::unique_ptr<cv::Mat> image;
    image        = std::make_unique<cv::Mat>(cv::imread(*resource_it));
    double scale = (static_cast<double>(output_rows_) / image->rows) / format.tiling;
    int tx       = (i % format.tiling) * output_cols_ / format.tiling;
    int ty       = (i / format.tiling) * output_rows_ / format.tiling;

    if (format.tiling == 1) {
      try {
        meta_tool::meta_tool meta;
        meta.read(*resource_it);
        if (meta.date().has_value() || meta.photographer().has_value() ||
            meta.world().has_value() || (meta.users().size() != 0)) {
          scale *= 0.8;
          put_meta_text(working, meta);
        }
      } catch (std::runtime_error e) {
      }
    }

    cv::Mat affine = (cv::Mat_<double>(2, 3) << scale, 0, tx, 0, scale, ty);
    cv::warpAffine(*image, working, affine, working.size(), cv::INTER_LINEAR,
                   cv::BORDER_TRANSPARENT);
    image.reset();
    thread_queue_ -= 1;
  };

  // bench
  auto rit            = resource_it;
  auto thread_elapsed = 0;
  auto pool_elapsed   = 0;
  for (int i = 0; i < 10; i++) {
    // init
    resource_it = rit;
    auto p      = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> ths;
    for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths_.rend();
         i++, resource_it++) {
      ths.push_back(std::thread(func, i, resource_it));
    }
    for (auto it = ths.begin(); it != ths.end(); it++) {
      it->join();
    }
    auto p_      = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(p_ - p).count();
    thread_elapsed += elapsed;
    std::cout << "thread " << elapsed << std::endl;

    // init
    resource_it   = rit;
    thread_queue_ = 0;
    auto q        = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < std::pow(format.tiling, 2) && resource_it != resource_paths_.rend();
         i++, resource_it++) {
      thread_queue_ += 1;
      thread_.post(boost::bind<void>(func, i, resource_it));
    }
    while (thread_queue_ != 0) {
    }
    auto q_       = std::chrono::high_resolution_clock::now();
    auto elapsed_ = std::chrono::duration_cast<std::chrono::milliseconds>(q_ - q).count();
    pool_elapsed += elapsed_;
    std::cout << "pool  " << elapsed_ << std::endl;
  }
  std::cout << "thread elapsed: " << thread_elapsed << std::endl;
  std::cout << "pool   elapsed: " << pool_elapsed << std::endl;

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
  std::unique_ptr<Magick::Image> image = std::make_unique<Magick::Image>(
      Magick::Geometry(output_cols_, output_rows_), Magick::Color(0, 0, 0));

  const int point_size = output_rows_ * 0.08;
  const cv::Point date_pos(0, output_rows_ * 0.8 + output_rows_ * 0.09);
  const cv::Point world_pos(0, output_rows_ * 0.8 + output_rows_ * 0.18);
  const int user_point_size = point_size / 2;
  cv::Point user_pos(output_cols_ * 0.8 + 10, user_point_size);

  // const cv::Scalar color(255, 255, 0);

  std::vector<Magick::Drawable> draw_list;

  draw_list.push_back(Magick::DrawableFont("Noto-Sans-CJK-JP"));
  draw_list.push_back(Magick::DrawableFillColor("white"));
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
  image->write(0, 0, output_cols_, output_rows_, "BGR", Magick::CharPixel, mat.data);
  image.reset();
}
} // namespace vrc_photo_streamer::photo
