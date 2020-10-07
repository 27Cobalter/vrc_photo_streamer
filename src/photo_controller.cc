#include <algorithm>
#include <cmath>
#include <iostream>

#include "photo_controller.h"
#include "photo_album.h"

namespace vrc_photo_streamer::controller {

photo::page_data photo_controller::current_page_ = {0, 3};
photo::page_data photo_controller::tiling_page_  = {0, 0};
photo::photo_album photo_controller::photo_album_;
size_t photo_controller::end = 0;

int photo_controller::find_images() {
  return photo_album_.find_images();
}

cv::Mat& photo_controller::get_frame() {
  if (tiling_page_.tiling == 0) {
    end          = photo_album_.find_images();
    tiling_page_ = current_page_;

    update(current_page_);
  }

  return photo_album_.get_frame();
}

void photo_controller::next() {
  end                       = photo_album_.find_images();
  photo::page_data new_page = current_page_;

  new_page.start += std::pow(new_page.tiling, 2);

  if (new_page.start >= end) new_page.start = 0;

  update(new_page);
}

void photo_controller::prev() {
  end                       = photo_album_.find_images();
  photo::page_data new_page = current_page_;

  new_page.start -= std::pow(new_page.tiling, 2);

  if (new_page.start < 0)
    new_page.start = std::trunc((end - 1) / new_page.tiling) * new_page.tiling;

  update(new_page);
}

void photo_controller::select(std::optional<int> num) {
  photo::page_data new_page = current_page_;

  if (num) {
    if (current_page_.tiling != 1) {
      tiling_page_ = current_page_;
    }

    new_page.start += std::clamp(num.value(), 0, static_cast<int>(end));
    new_page.tiling = 1;
    // std::cout << "new_page " << new_page.start << ", " << new_page.tiling << std::endl;
  } else {
    new_page = tiling_page_;
  }
  update(new_page);
}

void photo_controller::update(photo::page_data new_page) {
  current_page_ = new_page;
  std::cout << "update: " << new_page.start << ", " << new_page.tiling << std::endl;
  photo_album_.update(new_page);
}
} // namespace vrc_photo_streamer::controller
