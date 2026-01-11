#include <expected>
#include <optional>
#include <regex>
#include <string>

#include "uri.hh"

using namespace curi;

Path::Path(std::initializer_list<std::string_view> strings)
  : m_parts(strings.begin(), strings.end()) {};

std::expected<std::optional<Path>, unsigned>
Path::parse(std::string_view str)
{
  static std::regex path_reg("^(\\/[a-zA-Z0-9_-]+)+\\/?");

  if (str.size() == 0)
    return std::nullopt;

  std::cmatch match;

  if (not std::regex_search(str.begin(), str.end(), match, path_reg))
    return std::unexpected(0);

  if ((int)match[0].length() != (int)str.size())
    return std::unexpected(match[0].length());

  Path out({});

  /* start at 1 to skip the first / delimiter */
  unsigned idx = 1;
  while (idx < str.size()) {
    auto const front = str.substr(idx);
    auto const slice = front.substr(0, front.find_first_of('/'));
    out.m_parts.push_back(std::string(slice));
    idx += slice.size() + 1;
  }

  return out.canonical();
}

std::optional<Path>
Path::canonical() const
{
  Path out({});

  for (auto const& part : m_parts) {
    if (part == "..") {
      if (m_parts.empty())
        return std::nullopt;
      out.m_parts.pop_back();
    } else if (part.empty() or part == ".")
      continue;
    else
      out.m_parts.push_back(part);
  }

  return out;
}

std::optional<Path>
Path::parent() const
{
  if (m_parts.empty())
    return std::nullopt;

  return Path(m_parts.begin(), --m_parts.end());
}

std::expected<URI, unsigned>
URI::parse(std::string_view str)
{
  URI out{};

  static std::regex scheme_reg("^[a-zA-Z][-+.a-zA-Z0-9]*:");
  static std::regex authority(
    "^\\/\\/([_a-zA-Z0-9]+@)?([_a-zA-Z0-9\\.]+)(:[0-9]{0,5})?");

  unsigned idx{};

  std::cmatch match;
  if (not std::regex_search(str.begin(), str.end(), match, scheme_reg))
    return std::unexpected(0);

  out.scheme = match[0].str();
  idx += out.scheme.size();
  out.scheme.pop_back(); /* remove the : */

  if (not std::regex_search(str.begin() + idx, str.end(), match, authority))
    return std::unexpected(idx);

  out.authority = std::string{ match.str().substr(2) };
  idx += match.length() + 0; /* skip the // */

  auto const pathopt = Path::parse({ str.begin() + idx, str.end() });
  if (not pathopt)
    return std::unexpected(pathopt.error());

  out.path = *pathopt;
  return out;
}
