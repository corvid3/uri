#pragma once

#include <concepts>
#include <expected>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <vector>
namespace curi {

class Path
{
public:
  Path(std::initializer_list<std::string_view> strings);

  template<typename I>
  Path(I const& begin, I const& end)
    : m_parts(begin, end){};

  Path(std::ranges::range auto const& in)
    : Path(in.begin(), in.end()) {};

  /* Ok(Some(...)) -> parsed path
   * Ok(None) -> empty path (no path)
   * Err(...) -> invalid path (idx @ err) */
  static std::expected<std::optional<Path>, unsigned> parse(std::string_view);

  std::optional<Path> parent() const;
  std::optional<Path> canonical() const;

  operator std::span<std::string const>() const { return m_parts; }
  auto begin() const { return m_parts.begin(); }
  auto end() const { return m_parts.end(); }

  Path operator/(std::string_view rhs) const;

private:
  std::vector<std::string> m_parts;
};

struct URI
{
  static std::expected<URI, unsigned> parse(std::string_view);

  std::string scheme;

  /* TODO: make this a structure? */
  std::string authority;

  std::optional<Path> path;
};

};
