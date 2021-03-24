#include "book.hpp"
#include <boost/system/detail/errc.hpp>
#include <exception>
#include <filesystem>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

namespace xml {

using document_t = std::remove_pointer_t<htmlDocPtr>;
using document = std::unique_ptr<document_t, decltype(&xmlFreeDoc)>;

enum class type {
  node = XML_ELEMENT_NODE,
  text = XML_TEXT_NODE,
};

using node_t = xmlNode;
class node;

using property_t = xmlAttr;
class property;

std::string_view sv(const xmlChar* str) noexcept
{
  if (!str) {
    return {};
  }
  return { reinterpret_cast<const char*>(str) };
}

class property {
public:
  property(property_t* property) : property_(property)
  {}

  property(property&& other) = default;
  property(const property& other) = default;
  property& operator=(property&& other) = default;
  property& operator=(const property& other) = default;

  constexpr operator bool() const noexcept
  {
    return property_ != nullptr;
  }

  auto name() noexcept
  {
    return sv(property_->name);
  }

  property_t* operator->() noexcept
  {
    return property_;
  }

  node children() noexcept;

private:
  property_t* property_;
};

class node {
public:
  node(node_t* node) : node_(node)
  {}

  node(node&& other) = default;
  node(const node& other) = default;
  node& operator=(node&& other) = default;
  node& operator=(const node& other) = default;

  constexpr operator bool() const noexcept
  {
    return node_ != nullptr;
  }

  auto type() noexcept
  {
    return static_cast<xml::type>(node_->type);
  }

  auto name() noexcept
  {
    return sv(node_->name);
  }

  auto text() noexcept
  {
    return sv(node_->content);
  }

  auto content() noexcept
  {
    return sv(node_->content);
  }

  auto properties() noexcept
  {
    return property{ node_->properties };
  }

  auto operator->() noexcept
  {
    return node_;
  }

private:
  node_t* node_;
};

inline node property::children() noexcept
{
  return property_->children;
}

inline document create(std::string_view src)
{
  const auto data = src.data();
  const auto size = static_cast<int>(src.size());
  auto options = 0;
  options |= HTML_PARSE_NOBLANKS;
  options |= HTML_PARSE_NOERROR;
  options |= HTML_PARSE_NOWARNING;
  options |= HTML_PARSE_NONET;
  return { htmlReadMemory(data, size, nullptr, nullptr, options), xmlFreeDoc };
}

inline node get_root(document& document)
{
  return { xmlDocGetRootElement(document.get()) };
}

}  // namespace xml

class client {
public:
  client(std::string host) :
    io_context_(1),
    work_(io_context_.get_executor()),
    host_(std::move(host))
  {}

  client(client&& other) = delete;
  client(const client& other) = delete;
  client& operator=(client&& other) = delete;
  client& operator=(const client& other) = delete;

  void start()
  {
    thread_ = std::thread([this]() {
      io_context_.run();
    });
  }

  void stop()
  {
    const auto op = [this]() -> net::awaitable<void> {
      if (stream_) {
        beast::error_code ec;
        co_await stream_->async_shutdown(net::redirect_error(net::use_awaitable, ec));
        if (ec != ssl::error::stream_truncated) {
          std::cerr << "warning: socket shutdown failed: " << ec.message() << std::endl;
        }
        beast::get_lowest_layer(*stream_).close();
        stream_ = std::nullopt;
        context_ = std::nullopt;
      }
    };

    net::co_spawn(io_context_, op, net::detached);
    work_.reset();

    if (thread_.joinable()) {
      thread_.join();
    }
  }

  net::awaitable<std::string> get(const std::string& path)
  {
    beast::error_code ec;

    const auto op = [&]() noexcept -> net::awaitable<std::string> {
      co_return co_await get(path, ec);
    };

    const auto body = io_context_.get_executor() != co_await net::this_coro::executor
      ? co_await net::co_spawn(io_context_, op, net::use_awaitable)
      : co_await op();

    if (ec) {
      throw beast::system_error(ec, "get");
    }

    co_return body;
  }

private:
  net::awaitable<std::string> get(const std::string& path, beast::error_code& ec) noexcept
  {
    std::size_t step = 0;
    std::cout << path << ' ' << step++ << std::endl;
    if (!context_) {
      ssl::context context{ ssl::context::tlsv13_client };
      context.set_verify_mode(ssl::verify_none, ec);
      if (ec) {
        co_return std::string{};
      }
      context_ = std::move(context);
    }
    std::cout << path << ' ' << step++ << std::endl;

    if (!stream_) {
      beast::ssl_stream<beast::tcp_stream> stream{ io_context_, *context_ };
      if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.data())) {
        ec = { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
        co_return std::string{};
      }

      tcp::resolver resolver{ co_await net::this_coro::executor };
      const auto results = co_await resolver.async_resolve(
        host_.data(), "https", net::redirect_error(net::use_awaitable, ec));
      if (ec) {
        co_return std::string{};
      }

      co_await beast::get_lowest_layer(stream).async_connect(
        results, net::redirect_error(net::use_awaitable, ec));
      if (ec) {
        co_return std::string{};
      }
      co_await stream.async_handshake(
        ssl::stream_base::client, net::redirect_error(net::use_awaitable, ec));
      if (ec) {
        co_return std::string{};
      }
      stream_ = std::move(stream);
    }
    std::cout << path << ' ' << step++ << std::endl;

    http::request<http::string_body> request{ http::verb::get, path, 11 };
    request.set(http::field::host, host_);
    request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    co_await http::async_write(*stream_, request, net::redirect_error(net::use_awaitable, ec));
    if (ec) {
      co_return std::string{};
    }
    std::cout << path << ' ' << step++ << std::endl;

    beast::flat_buffer buffer;
    http::response_parser<http::string_body> parser;
    co_await http::async_read_header(
      *stream_, buffer, parser, net::redirect_error(net::use_awaitable, ec));
    if (ec) {
      co_return std::string{};
    }
    const auto status = parser.get().result();
    if (status != http::status::ok) {
      ec = http::make_error_code(http::error::bad_status);
      co_return std::string{};
    }
    std::cout << path << ' ' << step++ << std::endl;

    auto on_body = [&](std::uint64_t size, std::string_view data, beast::error_code& ec) {
      parser.get().body().append(data.data(), data.size());
      return data.size();
    };
    parser.on_chunk_body(on_body);
    std::cout << path << ' ' << step++ << std::endl;

    co_await http::async_read(*stream_, buffer, parser, net::redirect_error(net::use_awaitable, ec));
    if (ec) {
      co_return std::string{};
    }
    std::cout << path << ' ' << step++ << std::endl;
    co_return parser.get().body();
  }

  std::thread thread_;
  net::io_context io_context_;
  net::executor_work_guard<net::io_context::executor_type> work_;
  std::optional<ssl::context> context_;
  std::optional<beast::ssl_stream<beast::tcp_stream>> stream_;
  std::string host_;
};

class application {
public:
  application(int threads = static_cast<int>(std::thread::hardware_concurrency())) :
    client_("novelfull.com"),
    threads_(threads),
    io_context_(threads)
  {}

  application(application&& other) = delete;
  application(const application& other) = delete;
  application& operator=(application&& other) = delete;
  application& operator=(const application& other) = delete;

  int run(bool make) noexcept
  {
    try {
      client_.start();

      for (std::size_t i = 1; i <= 2302; i++) {
        net::co_spawn(io_context_, process(i, make), net::detached);
      }

      std::vector<std::thread> threads;
      threads.resize(static_cast<std::size_t>(threads_));
      for (int i = 1; i < threads_; i++) {
        threads.emplace_back([this]() {
          io_context_.run();
        });
      }

      io_context_.run();

      for (auto& thread : threads) {
        if (thread.joinable()) {
          thread.join();
        }
      }

      if (chapters_.empty()) {
        throw std::runtime_error("no chapters");
      }

      if (make) {
        std::string text;
        text.append("---\n" + read("res/book.yaml") + "...\n\n\n");

        for (const auto& chapter : chapters_) {
          text.append(chapter.second);
          text.append("\n\n");
        }

        if (text.size() > 1) {
          text.resize(text.size() - 1);
        }

        write("book.md", text);
      }
    }
    catch (const std::exception& e) {
      std::cerr << "error: " << e.what() << std::endl;
      result_ = EXIT_FAILURE;
    }
    catch (...) {
      std::cerr << "unexpected exception" << std::endl;
      result_ = EXIT_FAILURE;
    }

    client_.stop();
    return result_;
  }

private:
  net::awaitable<void> process(std::size_t index, bool make) noexcept
  {
    using std::filesystem::is_regular_file;
    using std::filesystem::last_write_time;

    try {
      const auto man = cache(index);
      const auto src = cache("src", index);
      const auto txt = cache("txt", index);

      // Manual override.
      if (is_regular_file(man)) {
        auto data = read(man);
        std::lock_guard lock{ mutex_ };
        chapters_[index] = std::move(data);
        co_return;
      }

      // Download source.
      if (!is_regular_file(src)) {
        write(src, co_await download(index));
      }

      // Use cached text file.
      if (is_regular_file(txt) && last_write_time(txt) >= last_write_time(src)) {
        auto data = read(txt);
        std::lock_guard lock{ mutex_ };
        chapters_[index] = std::move(data);
        co_return;
      }

      // Format source.
      {
        std::lock_guard lock{ mutex_ };
        std::cout << "transforming chapter " << index << " ..." << std::endl;
      }
      auto text = book_.format(index, parse(read(src)));
      write(txt, text);

      if (make) {
        std::lock_guard lock{ mutex_ };
        chapters_[index] = std::move(text);
      }
    }
    catch (const std::exception& e) {
      std::lock_guard lock{ mutex_ };
      std::cerr << "chapter " << index << " error: " << e.what() << std::endl;
      result_ = EXIT_FAILURE;
    }
    catch (...) {
      std::lock_guard lock{ mutex_ };
      std::cerr << "chapter " << index << " error: unexpected exception" << std::endl;
      result_ = EXIT_FAILURE;
    }
    co_return;
  }

  net::awaitable<std::string> download(std::size_t index)
  {
    {
      std::lock_guard lock{ mutex_ };
      std::cout << "downloading chapter " << index << " ..." << std::endl;
    }
    const auto path = "/reverend-insanity/chapter-" + std::to_string(index) + ".html";
    const auto data = co_await client_.get(path);
    co_return data;
  }

  std::vector<std::string> parse(const std::string& data)
  {
    auto document = xml::create(data);

    std::vector<std::string> text;
    std::string line;

    const std::function<void(xml::node)> parse_chapter = [&](xml::node node) {
      for (auto cur = node; cur; cur = cur->next) {
        if (cur.type() == xml::type::text) {
          line.append(cur.text());
          line.push_back(' ');
        }
        parse_chapter(cur->children);
        if (cur.type() == xml::type::node && (cur.name() == "p" || cur.name() == "br")) {
          line = book::trim(std::move(line));
          if (!line.empty()) {
            text.push_back(std::move(line));
            line.clear();
          }
        }
      }
    };

    const std::function<bool(xml::node)> parse_document = [&](xml::node node) {
      for (auto cur = node; cur; cur = cur->next) {
        if (cur.type() == xml::type::node) {
          if (cur.name() == "div") {
            for (auto prop = cur.properties(); prop; prop = prop->next) {
              if (prop.name() == "id") {
                if (auto child = prop.children()) {
                  if (child.content() == "chapter-content") {
                    parse_chapter(cur->children);
                    return false;
                  }
                }
              }
            }
          }
        }
        if (!parse_document(cur->children)) {
          return false;
        }
      }
      return true;
    };

    parse_document(xml::get_root(document));

    if (text.empty()) {
      throw std::runtime_error("missing content");
    }
    return text;
  }

  static std::filesystem::path cache(std::size_t index)
  {
    const auto filename = std::to_string(index) + ".md";
    return std::filesystem::path{ "cache" } / filename;
  }

  static std::filesystem::path cache(std::string_view directory, std::size_t index)
  {
    const auto filename = [&]() {
      if (directory == "src") {
        return std::to_string(index) + ".html";
      }
      return std::to_string(index) + ".md";
    }();
    return std::filesystem::path{ "cache" } / directory / filename;
  }

  static std::string read(const std::filesystem::path& file)
  {
    std::ifstream is{ file, std::ios::binary };
    if (is.bad() || is.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "read open error: " + file.string() };
    }
    is.seekg(0, std::ios::end);
    if (is.bad() || is.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "seek error: " + file.string() };
    }
    std::string data;
    data.resize(static_cast<std::size_t>(is.tellg()));
    is.seekg(0, std::ios::beg);
    if (is.bad() || is.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "seek error: " + file.string() };
    }
    is.read(data.data(), data.size());
    if (is.bad() || is.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "read error: " + file.string() };
    }
    return data;
  }

  static void write(const std::filesystem::path& file, const std::string& data)
  {
    if (file.has_parent_path()) {
      std::filesystem::create_directories(file.parent_path());
    }
    std::ofstream os{ file, std::ios::binary };
    if (os.bad() || os.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "write open error: " + file.string() };
    }
    os.write(data.data(), data.size());
    if (os.bad() || os.fail()) {
      const auto ec = std::make_error_code(static_cast<std::errc>(errno));
      throw std::system_error{ ec, "write error: " + file.string() };
    }
  }

  book book_;
  client client_;
  int threads_ = 1;
  std::mutex mutex_;
  net::io_context io_context_;
  std::map<std::size_t, std::string> chapters_;

  int result_ = EXIT_SUCCESS;
};

int main(int argc, char** argv)
{
  try {
    application app;
    return app.run(argc > 1 && std::string_view{ argv[1] } == "book");
  }
  catch (std::exception const& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
