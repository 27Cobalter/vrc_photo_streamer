#include "photo_controller.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "photo_album.h"

namespace vrc_photo_streamer::controller {

photo_controller::photo_controller(int argc, char** argv, int output_cols, int output_rows) {
  photo_album_  = std::make_unique<photo::photo_album>(argc, argv, output_cols, output_rows);
  current_page_ = {0, 3};
  tiling_page_  = current_page_;

  end_ = photo_album_->find_images();
  update(current_page_);
}

int photo_controller::find_images() {
  return photo_album_->find_images();
}

std::shared_ptr<cv::Mat> photo_controller::get_frame_ptr() {
  return photo_album_->get_frame_ptr();
}

void photo_controller::next() {
  end_                      = photo_album_->find_images();
  photo::page_data new_page = current_page_;

  new_page.start -= tile2(current_page_);

  if (new_page.start < 0)
    new_page.start = std::trunc((end_ - 1) / tile2(new_page)) * tile2(new_page);

  update(new_page);
}

void photo_controller::prev() {
  end_                      = photo_album_->find_images();
  photo::page_data new_page = current_page_;

  new_page.start += tile2(current_page_);

  if (new_page.start >= end_) new_page.start = 0;

  update(new_page);
}

void photo_controller::head() {
  end_                      = photo_album_->find_images();
  photo::page_data new_page = current_page_;

  new_page.start = 0;
  new_page.tiling = 3;

  update(new_page);
}

void photo_controller::select(std::optional<int> num) {
  photo::page_data new_page = current_page_;

  if (num) {
    if (current_page_.tiling != 1) {
      tiling_page_ = current_page_;
    }

    new_page.start += std::clamp(num.value(), 0, tile2(current_page_) - 1);
    new_page.tiling = 1;
    // std::cout << "new_page " << new_page.start << ", " << new_page.tiling << std::endl;
  } else {
    new_page = tiling_page_;
    new_page.start =
        std::trunc((current_page_.start) / tile2(tiling_page_)) * tile2(tiling_page_);
  }
  update(new_page);
}

void photo_controller::update(photo::page_data new_page) {
  current_page_ = new_page;
  std::cout << "update: " << new_page.start << ", " << new_page.tiling << std::endl;
  photo_album_->update(new_page);
}

int photo_controller::tile2(photo::page_data page) {
  return std::pow(page.tiling, 2);
}
} // namespace vrc_photo_streamer::controller
