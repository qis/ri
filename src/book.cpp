#include "book.hpp"

book::book() : ch_("Chapter[^0-9]*([0-9]+)[^a-zA-Z]*(.*)", "Chapter&nbsp;$1: $2")
{
  /*
  ts_.emplace_back("([,.?!;:])([^ ])", "$1 $2");
  ts_.emplace_back("\\s([,.?!;:])", "$1");

  ts_.emplace_back("(“)+", "“");
  ts_.emplace_back("(”)+", "”");
  ts_.emplace_back("“\\s", "“");
  ts_.emplace_back("\\s”", "”");

  ts_.emplace_back("(‘)+", "‘");
  ts_.emplace_back("(’)+", "’");
  ts_.emplace_back("‘\\s", "‘");
  ts_.emplace_back("\\s’", "’");

  ts_.emplace_back("”“", "” “");

  ts_.emplace_back("\\s《", "《");
  ts_.emplace_back("》\\s", "》");

  ts_.emplace_back("F\\*ck", "Fuck");
  ts_.emplace_back("f\\*ck", "fuck");
  ts_.emplace_back("b\\*tch", "bitch");
  ts_.emplace_back("b\\*llshit", "bullshit");
  ts_.emplace_back("gr\\*ndmother", "grandmother");
  ts_.emplace_back("mother\\*\\*\\*\\*ing", "motherfucking");
  */
  ts_.emplace_back("Translator\\s*:.*(AtlasStudios|Atlas Studios|ChibiGeneral|Chibi General|Sigma|Skyfarrow)", "");
  ts_.emplace_back("Editor\\s*:.*(AtlasStudios|Atlas Studios|ChibiGeneral|Chibi General|Sigma|Skyfarrow)", "");
}

std::string book::format(std::size_t index, std::vector<std::string> lines)
{
  for (auto it = lines.begin(); it != lines.end(); ++it) {
    auto& line = *it;

    replace(line, "|", "");
    replace(line, "[email protected]", " ");
    replace(line, " Updated by NovelFull.Com)", "");
    replace(line, "/ NovelFull.Com", "");
    replace(line, "/ update by NovelFull.Com", "");
    replace(line, "( NovelFull )", "");
    replace(line, "– You’re reading on NovelFull .Tks!", "");
    replace(line, "You’re reading on NovelFull Thanks!", "");
    replace(line, "Visit on our NovelFullFang", "");
    replace(line, "Read more chapter on NovelFull", "");
    replace(line, "Read comics on our webnovel.live", "");
    replace(line, "Read more chapter at vipnovel", "");
    replace(line, "Reverend Insanity Prev Chapter Next Chapter", "");
    replace(line, "COMMENT", "");
    replace(line, "F*ck", "Fuck");
    replace(line, "f*ck", "fuck");
    replace(line, "b*tch", "bitch");
    replace(line, "b*llshit", "bullshit");
    replace(line, "gr*ndmother", "grandmother");
    replace(line, "mother****ing", "motherfucking");

    if (line.starts_with("*This chapter") && line.ends_with("with the translations.")) {
      line.clear();
    }

    if (line.starts_with("*Donations are") && line.ends_with("(+_+)/")) {
      line.clear();
    }

    line = trim(std::move(line));

    for (const auto& tr : ts_) {
      std::string buffer;
      std::regex_replace(std::back_inserter(buffer), line.begin(), line.end(), tr.match, tr.replace);
      line = std::move(buffer);
    }

    replace(line, "Editor:Gu Zhen Ren", "");
    replace(line, "ChibiGeneral", "");
    replace(line, "– Skyfarrow", "");
    replace(line, "TRANSLATOR ", "Translator’s Thoughts: ");
    replace(line, "Translator Note: ", "Translator’s Thoughts: ");
    replace(line, "Translator’s Thoughts   ", "Translator’s Thoughts: ");

    line = trim(std::move(line));
  }

  lines.erase(
    std::remove_if(
      lines.begin(),
      lines.end(),
      [](const auto& s) {
        return s.empty();
      }),
    lines.end());

  if (lines.empty()) {
    throw std::runtime_error("missing content");
  }

  std::string buffer;
  std::regex_replace(std::back_inserter(buffer), lines[0].begin(), lines[0].end(), ch_.match, ch_.replace);
  lines[0] = std::move(buffer);

  std::string text;
  text.append("# ");
  text.append(lines[0]);
  text.append("\n\n");
  for (std::size_t i = 1, max = lines.size(); i < max; i++) {
    if (lines[i] == "…" || lines[i] == "……" || lines[i] == "*****") {
      text.append("<div style=\"text-align: center;\">• • •</div>\n\n");
    } else {
      text.append(lines[i]);
      text.append("\n\n");
    }
  }
  if (text.size() > 1) {
    text.resize(text.size() - 1);
  }

  return text;
}

std::string book::trim(std::string s)
{
  static const transformation tr{ "\\s\\s+", "" };

  replace(s, " ", " ");
  replace(s, "‌", "");

  std::string buffer;
  std::regex_replace(std::back_inserter(buffer), s.begin(), s.end(), tr.match, tr.replace);
  s = std::move(buffer);

  if (const auto pos = s.find_first_not_of(" \n\r\t\f\v"); pos == std::string_view::npos) {
    return {};
  } else if (pos) {
    s = s.substr(pos);
  }

  if (const auto pos = s.find_last_not_of(" \n\r\t\f\v"); pos == std::string_view::npos) {
    return {};
  } else {
    s = s.substr(0, pos + 1);
  }
  return s;
}

void book::replace(std::string& s, std::string_view search, std::string_view replace)
{
  while (true) {
    if (const auto pos = s.find(search); pos != std::string::npos) {
      s.replace(pos, search.size(), replace);
    } else {
      break;
    }
  }
}
