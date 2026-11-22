#include "uri.hh"

#include <optional>

auto
curi::parse_domain(std::string_view const host)
  -> std::optional<std::list<std::string>>
{
  std::list<std::string> out;
  size_t                 i = 0;

  std::string working{};

  while (i < host.size()) {
    char const c = host.at(i);

    if (c == '.') {
      if (working.size() == 0)
        return std::nullopt;
      out.push_front(working);
      working.clear();
    } else {
      working.push_back(c);
    }

    i++;
  }

  if (working.size() == 0)
    return std::nullopt;
  out.push_front(working);
  working.clear();

  return out;
}
