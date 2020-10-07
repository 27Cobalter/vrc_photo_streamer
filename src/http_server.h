#ifndef VRC_PHOTO_STREAMER_HTTP_SERVER_H
#define VRC_PHOTO_STREAMER_HTTP_SERVER_H

#include <chrono>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace vrc_photo_streamer::http {

namespace beast  = boost::beast;
namespace http   = beast::http;
namespace asio   = boost::asio;
namespace chrono = std::chrono;
using tcp        = boost::asio::ip::tcp;

class http_server {
public:
  void tcp_server(tcp::acceptor& acceptor, tcp::socket& socket);
  void run();
};

class http_connection : public std::enable_shared_from_this<http_connection> {
public:
  http_connection(tcp::socket socket);
  void start();

private:
  tcp::socket socket_;
  beast::flat_buffer buffer_{8192};
  http::request<http::dynamic_body> request_;
  http::response<http::dynamic_body> response_;
  asio::steady_timer deadline_{socket_.get_executor(), chrono::seconds(60)};
  void read_request();
  void process_request();
  void write_response();
  void check_deadline();
};
} // namespace vrc_photo_streamer::http
#endif
