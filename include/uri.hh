#pragma once

#include <concepts>
#include <expected>
#include <filesystem>
#include <initializer_list>
#include <list>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <vector>
namespace curi
{

using host = std::variant<std::string, uint32_t>;

struct authority {
  std::optional<std::string> username;
  std::optional<unsigned>    port;

  /* in canonical order
   * i.e. "foo.net" -> "net", "foo"
   * */
  curi::host host;
};

struct URI {
  /* This converts percent encoded values into their byte representation. */
  static auto parse(std::string_view) -> std::optional<URI>;

  std::optional<std::string>     scheme;
  std::optional<curi::authority> authority;
  std::string                    path;
};

/* splits an internet domain name into individual parts, in canonical order
 * that is, "foo.bar.net" -> "net","foo","bar"
 */
auto
parse_domain(std::string_view const host)
  -> std::optional<std::list<std::string>>;

};
