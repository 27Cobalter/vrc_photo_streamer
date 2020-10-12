#include "http_server.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "photo_controller.h"

namespace vrc_photo_streamer::http {

http_server::http_server(std::shared_ptr<controller::photo_controller> controller)
    : controller_(controller){};

void http_server::tcp_server(tcp::acceptor& acceptor, tcp::socket& socket) {
  acceptor.async_accept(socket, [&](beast::error_code ec) {
    if (!ec) {
      std::make_shared<http_connection>(std::move(socket), controller_)->start();
    }
    tcp_server(acceptor, socket);
  });
}

void http_server::run() {
  try {
    asio::ip::address address = boost::asio::ip::make_address("0.0.0.0");
    asio::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {address, 8555}};
    tcp::socket socket{ioc};
    tcp_server(acceptor, socket);
    std::cout << "http ready at http://127.0.0.1:8555" << std::endl;

    ioc.run();
  } catch (std::exception const& e) {
    throw e;
  }
}

http_connection::http_connection(tcp::socket socket,
                                 std::shared_ptr<controller::photo_controller> controller)
    : socket_(std::move(socket)), controller_(controller){};
void http_connection::start() {
  read_request();
  check_deadline();
}

void http_connection::read_request() {
  auto self = shared_from_this();

  http::async_read(socket_, buffer_, request_,
                   [self](beast::error_code ec, std::size_t bytes_transferred) {
                     boost::ignore_unused(bytes_transferred);
                     if (!ec) {
                       self->process_request();
                     }
                   });
}

void http_connection::process_request() {
  response_.version(request_.version());
  response_.keep_alive(false);

  // std::cout << "base:\n" << request_.base() << "\n"<< std::endl;
  if (request_.method() == http::verb::get) {
    // TODO: ここらへんにいろいろ書いてく
    if (request_.target() == "/next") {
      controller_->next();
    } else if (request_.target() == "/prev") {
      controller_->prev();
    } else if (request_.target() == "/head") {
      controller_->head();
    } else if (request_.target().substr(0, 7) == "/select") {
      // std::cout << request_.target() << std::endl;
      if (auto index = request_.target().find("?num="); index != std::string::npos) {
        try {
          int num = std::stoi(std::string(request_.target().substr(index + 5)));
          controller_->select(num);
        } catch (std::invalid_argument e) {
        }
      } else {
        controller_->select(std::nullopt);
      }
    }
  }

  // 404で返すとPanoramaにキャッシュされない
  response_.result(http::status::not_found);
  response_.set(http::field::content_type, "text/plain");
  beast::ostream(response_.body()) << "File not found.\n";
  response_.set(http::field::server, "Beast");

  write_response();
}

void http_connection::write_response() {
  auto self = shared_from_this();

  response_.content_length(response_.body().size());

  http::async_write(socket_, response_, [self](beast::error_code ec, std::size_t) {
    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
    self->deadline_.cancel();
  });
}

void http_connection::check_deadline() {
  auto self = shared_from_this();

  deadline_.async_wait([self](beast::error_code ec) {
    if (!ec) {
      self->socket_.close(ec);
    }
  });
}
} // namespace vrc_photo_streamer::http
