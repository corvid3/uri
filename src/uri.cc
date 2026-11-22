#include "uri.hh"

#include <concepts>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <regex>
#include <string>

using namespace curi;

namespace
{

auto
isasciialpha(char const c) -> bool
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

auto
isasciidigit(char const c) -> bool
{ return c >= '0' && c <= '9'; }

class parser
{
public:
  explicit parser(std::string_view in)
    : m_str(in) {};

  [[nodiscard]]
  auto
  peek() const -> std::optional<char>
  {
    if (m_idx >= m_str.size())
      return std::nullopt;
    return m_str.at(m_idx);
  }

  [[nodiscard]]
  auto
  starts_with(std::string_view view) const
  { return m_str.substr(m_idx).starts_with(view); }

  auto
  advance()
  { m_idx++; }

private:
  std::string_view m_str;
  size_t           m_idx{};
};

auto
expect(parser& p, char const h) -> bool
{
  auto const c = p.peek();
  if (not c)
    return false;

  if (*c == h)
    return p.advance(), true;

  return false;
}

auto
parse_hex(parser& p) -> std::optional<char>
{
  auto const c = p.peek();
  if (not c)
    return std::nullopt;

  if (isasciidigit(*c) or (*c >= 'a' and *c <= 'f')
      or (*c >= 'A' and *c <= 'F'))
    return p.advance(), *c;

  return std::nullopt;
}

auto
parse_percent(parser& p) -> std::optional<char>
{
  if (not expect(p, '%'))
    return std::nullopt;

  auto const first  = parse_hex(p);
  auto const second = parse_hex(p);

  if (not first or not second)
    return std::nullopt;

  return (static_cast<unsigned int>(*first) << 4U)
       | static_cast<unsigned int>(*second);
}

auto
parse_digit(parser& p) -> std::optional<char>
{
  auto const c = p.peek();
  if (not c)
    return std::nullopt;

  if (isasciidigit(*c))
    return p.advance(), c;
  return std::nullopt;
}

auto
parse_unreserved(parser& p) -> std::optional<char>
{
  auto const c = p.peek();
  if (not c)
    return std::nullopt;

  if (isasciialpha(*c) or isasciidigit(*c) or *c == '-' or *c == '.'
      or *c == '_' or *c == '~')
    return p.advance(), c;

  return std::nullopt;
}

auto
parse_subdelim(parser& p) -> std::optional<char>
{
  auto const c = p.peek();
  if (not c)
    return std::nullopt;

  switch (*c) {
  case '!':
  case '$':
  case '&':
  case '\'':
  case '(':
  case ')':
  case '*':
  case '+':
  case ',':
  case ';':
  case '=' : return p.advance(), *c;
  default  : return std::nullopt;
  }

  __builtin_unreachable();
}

auto
parse_pchar(parser& p) -> std::optional<char>
{
  return parse_unreserved(p)
    .or_else([&] -> std::optional<char> {
      return parse_percent(p);
    })
    .or_else([&] -> std::optional<char> {
      return parse_subdelim(p);
    })
    .or_else([&] -> std::optional<char> {
      auto const c = p.peek();
      if (not c)
        return std::nullopt;

      if (*c == '@' or *c == ':')
        return p.advance(), c;
      return std::nullopt;
    });
}

auto
parse_scheme(parser& p) -> std::optional<std::string>
{
  auto const is_scheme_char = [](char const c) -> bool {
    return isasciialpha(c) || isasciidigit(c) || c == '+' || c == '-'
        || c == '.';
  };

  {
    auto const c = p.peek();
    if (not c)
      return std::nullopt;
    if (not is_scheme_char(*c))
      return std::nullopt;
  }

  std::string out;
  while (auto const c = p.peek()) {
    if (is_scheme_char(*c)) {
      out += *c;
      p.advance();
    } else {
      break;
    }
  }

  return out;
}

auto
parse_number(parser& p) -> std::optional<int>
{
  int out = 0;
  while (auto const c = parse_digit(p))
    out *= 10U, out += *c - '0';
  return out;
}

auto
parse_ipv4(parser& p) -> std::optional<uint32_t>
{
  auto const a = parse_number(p);
  if (not a or not expect(p, '.'))
    return std::nullopt;
  auto const b = parse_number(p);
  if (not b or not expect(p, '.'))
    return std::nullopt;
  auto const c = parse_number(p);
  if (not c or not expect(p, '.'))
    return std::nullopt;
  auto const d = parse_number(p);
  if (not d)
    return std::nullopt;

  if (*a >= std::numeric_limits<uint8_t>::max())
    return std::nullopt;
  if (*b >= std::numeric_limits<uint8_t>::max())
    return std::nullopt;
  if (*c >= std::numeric_limits<uint8_t>::max())
    return std::nullopt;
  if (*d >= std::numeric_limits<uint8_t>::max())
    return std::nullopt;

  return static_cast<uint32_t>(*a) << 24U | static_cast<uint32_t>(*b) << 16U
       | static_cast<uint32_t>(*c) << 8U | static_cast<uint32_t>(*d);
}

auto
parse_username(parser& p) -> std::optional<std::string>
{
  /* make a copy for backtracking in case there's no username */
  parser copy(p);

  auto const parse_userinfo_char = [&] -> std::optional<char> {
    return parse_unreserved(copy).or_else([&] {
      return parse_percent(copy);
    });
  };

  std::string out;

  while (auto const c = parse_userinfo_char())
    out += *c;

  if (not expect(copy, '@'))
    return std::nullopt;

  p = copy;
  return out;
}

auto
parse_regname(parser& p) -> std::optional<std::string>
{
  std::string out;

  auto const parse_char = [&] -> std::optional<char> {
    return parse_unreserved(p)
      .or_else([&] -> std::optional<char> {
        return parse_percent(p);
      })
      .or_else([&] -> std::optional<char> {
        return parse_subdelim(p);
      });
  };

  while (auto const c = parse_char())
    out += *c;

  if (out.size() == 0)
    return std::nullopt;

  return out;
}

auto
parse_host(parser& p) -> std::optional<host>
{
  parser copy(p);

  if (auto const ip = parse_ipv4(copy); ip) {
    p = copy;
    return { *ip };
  }

  return { parse_regname(p) };
}

auto
parse_port(parser& p) -> std::optional<int>
{
  if (not expect(p, ':'))
    return std::nullopt;

  return parse_number(p);
}

auto
parse_authority(parser& p) -> std::optional<authority>
{
  auto const username = parse_username(p);
  auto const host     = parse_host(p);
  auto const port     = parse_port(p);

  if (not host)
    return std::nullopt;

  return authority{ .username = username, .port = port, .host = *host };
}

auto
parse_segment(parser& p) -> std::optional<std::string>
{
  std::string out;

  while (auto const c = parse_pchar(p))
    out += *c;

  if (out.size() == 0)
    return std::nullopt;

  return out;
}

auto
parse_path_abempty(parser& p) -> std::optional<std::string>
{
  std::string out;

  while (expect(p, '/')) {
    auto const what = parse_segment(p);
    if (not what)
      break;

    out += "/" + *what;
  }

  return out;
}

auto
parse_hier_part(parser& p)
  -> std::optional<std::pair<std::optional<authority>, std::string>>
{
  if (p.starts_with("//")) {
    p.advance();
    p.advance();

    auto const authority = parse_authority(p);
    if (not authority)
      return std::nullopt;

    auto const path = parse_path_abempty(p);
    if (not path)
      return std::nullopt;

    return std::pair{ authority, *path };
  }

  return std::nullopt;
}

}

auto
URI::parse(std::string_view str) -> std::optional<URI>
{
  parser p{ str };

  auto const scheme = parse_scheme(p);
  if (not scheme)
    return std::nullopt;

  if (not expect(p, ':'))
    return std::nullopt;

  auto const part = parse_hier_part(p);
  if (not part)
    return std::nullopt;

  URI out;
  out.authority = part->first;
  out.path      = part->second;
  out.scheme    = *scheme;

  return out;
}
