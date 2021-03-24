

class book {
public:
  struct transformation {
    static constexpr auto common = std::regex_constants::ECMAScript | std::regex_constants::optimize;

    std::regex match;
    std::string replace;

    transformation(const char* match, std::string replace, bool noignore = true)
      : match(match, common | (noignore ? std::regex_constants::icase : common)),
        replace(std::move(replace))
    {}

    transformation(transformation&& other) = default;
    transformation(const transformation& other) = default;
    transformation& operator=(transformation&& other) = default;
    transformation& operator=(const transformation& other) = default;
  };

  book();

  book(book&& other) = delete;
  book(const book& other) = delete;
  book& operator=(book&& other) = delete;
  book& operator=(const book& other) = delete;

  std::string format(std::size_t index, std::vector<std::string> lines);

  static std::string trim(std::string s);
  static void replace(std::string& s, std::string_view search, std::string_view replace);

private:
  transformation ch_;
  std::vector<transformation> ts_;
};
