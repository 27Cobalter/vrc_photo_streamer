#ifndef VRC_PHOTO_STREAMER_VRC_META_TOOL_H
#define VRC_PHOTO_STREAMER_VRC_META_TOOL_H

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include <png.h>

namespace vrc_photo_streamer::meta_tool {

namespace filesystem = std::filesystem;

typedef struct {
  // TODO: clang++でchrono::local_seconds周りの実装が充実してきたらこっちにする
  std::optional<std::string> date;
  std::optional<std::string> photographer;
  std::optional<std::string> world;
  std::map<std::string, std::optional<std::string>> users;
} meta_data;

class chunk_util {
public:
  chunk_util(filesystem::path path);
  ~chunk_util();
  decltype(auto) read();
  // decltype(auto) write();
  // decltype(auto) create_chunk();

private:
  decltype(auto) parse_chunk(png_unknown_chunk chunk);
  std::FILE* fp;
  png_structp png_ptr;
  png_infop info_ptr;
  png_infop end_ptr;
};

class meta_tool {
public:
  std::optional<std::string> date() const;
  std::optional<std::string> photographer() const;
  std::optional<std::string> world() const;
  std::map<std::string, std::optional<std::string>> users() const;
  // decltype(auto) users_begin() const;
  // decltype(auto) users_end() const;
  void set_date(std::optional<std::string> date);
  void set_photographer(std::optional<std::string> photographer);
  void set_world(std::optional<std::string> world);
  void add_user(std::string user);
  void delete_user(std::string user_name);
  void clear_users();
  void read(filesystem::path path);

private:
  meta_data data_;
};
} // namespace vrc_photo_streamer::meta_tool
#endif
