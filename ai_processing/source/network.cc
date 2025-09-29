// SPDX-FileCopyrightText: 2024 Artur Bać
// SPDX-License-Identifier: MIT

#ifndef Q_MOC_RUN
#include "network.h"
#include <aiprocess/log.h>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fmt/core.h>

//
// https://api.openai.com/v1/engines/gpt-3.5-turbo/completions
// curl -X POST
//      -H "Authorization: Bearer YOUR_API_KEY"
//      -H "Content-Type: application/json"
//      -d '{"prompt": "Hello, world!", "max_tokens": 5}'
//      https://api.openai.com/v1/engines/gpt-3.5-turbo/completions
//

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
namespace ssl = net::ssl;        // from <boost/asio/ssl.hpp>

static auto to_string(http::response<http::dynamic_body> const & response) -> std::string
  {
  // serialize the header part of the response
  std::string result;
  result.reserve(512);
  for(auto const & field: response.base())
    {
    result.append(field.name_string());
    result.append(": ");
    result.append(field.value());
    result.append("\r\n");
    }
  result.append("\r\n");

  // Append the body content
  result.append(beast::buffers_to_string(response.body().data()));

  return result;
  }

static auto to_string(http::request<http::string_body> const & req) -> std::string
  {
  std::string request_text;

  // Add the request line
  request_text += req.method_string();
  request_text += " ";
  request_text += req.target();
  request_text += " ";
  request_text += req.version() == 11 ? "HTTP/1.1" : "HTTP/1.0";
  request_text += "\r\n";

  // Add the headers
  for(auto const & field: req.base())
    {
    request_text += field.name_string();
    request_text += ": ";
    request_text += field.value();
    request_text += "\r\n";
    }

  // End of headers
  request_text += "\r\n";

  // Add the body
  request_text += req.body();

  return request_text;
  }

using namespace aiprocess;

// Connection class to handle both SSL and non-SSL connections
class Connection {
public:
    Connection(std::string_view host, std::string_view port, bool use_ssl)
        : ioc_(), resolver_(ioc_), use_ssl_(use_ssl), host_(host) {
        auto const results = resolver_.resolve(host, port);

        if (use_ssl_) {
            ctx_ = std::make_unique<ssl::context>(ssl::context::tls_client);
            ctx_->set_default_verify_paths();
            ssl_stream_ = std::make_unique<ssl::stream<tcp::socket>>(ioc_, *ctx_);

            if (!SSL_set_tlsext_host_name(ssl_stream_->native_handle(), host_.data())) {
                throw std::runtime_error(fmt::format("Failed to set SNI Hostname: {}",
                    net::error::get_ssl_category().message(::ERR_get_error())));
            }

            net::connect(ssl_stream_->next_layer(), results.begin(), results.end());
            ssl_stream_->handshake(ssl::stream_base::client);
        } else {
            tcp_socket_ = std::make_unique<tcp::socket>(ioc_);
            net::connect(*tcp_socket_, results.begin(), results.end());
        }
    }

    template<typename Request>
    void write(Request const& req) {
        if (use_ssl_) {
            http::write(*ssl_stream_, req);
        } else {
            http::write(*tcp_socket_, req);
        }
    }

    template<typename Response>
    void read(beast::flat_buffer& buffer, Response& res) {
        if (use_ssl_) {
            http::read(*ssl_stream_, buffer, res);
        } else {
            http::read(*tcp_socket_, buffer, res);
        }
    }

    void shutdown() {
        beast::error_code ec;
        if (use_ssl_) {
            ssl_stream_->shutdown(ec);
            if (ec == net::error::eof || ec == ssl::error::stream_truncated) {
                ec = {};
            }
        } else {
            tcp_socket_->shutdown(tcp::socket::shutdown_both, ec);
        }
        if (ec && ec != net::error::eof) {
            throw std::runtime_error(fmt::format("Shutdown error: {}", ec.what()));
        }
    }

private:
    net::io_context ioc_;
    tcp::resolver resolver_;
    bool use_ssl_;
    std::string host_;
    std::unique_ptr<ssl::context> ctx_;
    std::unique_ptr<ssl::stream<tcp::socket>> ssl_stream_;
    std::unique_ptr<tcp::socket> tcp_socket_;
};

auto send_text_to_gpt(
  std::string_view host,
  std::string_view port,
  bool use_ssl,
  std::string_view target,
  std::string_view api_key,
  std::string_view text,
  int version
) -> expected<std::string, send_text_to_gpt_error>
  {
    // Set up the request. We use a string for the body.
    http::request<http::string_body> req{http::verb::post, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    req.set(http::field::content_type, "application/json");
    req.set(http::field::authorization, std::string("Bearer ").append(api_key));
    req.body() = text;
    req.prepare_payload();

    {
      auto http_message_string{to_string(req)};
      snpt::info("{}", http_message_string);
    }

    try {
        Connection conn(host, port, use_ssl);

        // Send the HTTP request to the remote host
        conn.write(req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        conn.read(buffer, res);

        {
            auto http_response_text{to_string(res)};
            snpt::info("{}", http_response_text);
        }

        // Gracefully close the connection
        conn.shutdown();

        return beast::buffers_to_string(res.body().data());
  }
  catch(std::exception const & e)
  {
  li::error("Network Error:\n{}", e.what());
  return unexpected{send_text_to_gpt_error::other_exception};
  }
  }
#if 0
auto parse(std::string_view url) -> url_components
  {
  url_components components;

  auto scheme_end = url.find("://");
  if(scheme_end != std::string_view::npos)
    {
    components.scheme = url.substr(0, scheme_end);
    url.remove_prefix(scheme_end + 3);  // Skip "://"
    }
  else
    {
    throw std::runtime_error("URL Scheme not found");
    }

  auto host_end = url.find('/');
  if(host_end != std::string_view::npos)
    {
    components.target = url.substr(host_end);
    }
  else
    {
    components.target = "/";  // Default target
    host_end = url.length();
    }

  auto port_start = url.find(":", 0, host_end);
  if(port_start != std::string_view::npos)
    {
    components.host = url.substr(0, port_start);
    components.port = url.substr(port_start + 1, host_end - port_start - 1);
    }
  else
    {
    components.host = url.substr(0, host_end);
    // Assign default port based on scheme
    components.port = components.scheme == "https" ? "443" : "80";
    }

  return components;
  }
#endif
#endif
