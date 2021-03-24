#pragma once

#ifdef _WIN32
#  include <windows.h>
#endif

#include <algorithm>
#include <atomic>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

#include <libxml/HTMLparser.h>
