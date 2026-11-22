#include "uri.hh"

#include <print>
#include <variant>

int
main(int argc, char** argv)
{
  if (argc < 2)
    exit(1);

  auto const uri = curi::URI::parse(argv[1]);

  if (not uri) {
    std::println("invalid uri parse");
  } else {
    if (uri->scheme)
      std::println("scheme: {}", *uri->scheme);

    if (uri->authority) {
      if (std::holds_alternative<std::string>(uri->authority->host)) {
        std::println("host: {}", std::get<std::string>(uri->authority->host));
        auto const list
          = curi::parse_domain(std::get<std::string>(uri->authority->host));
        for (auto const& name : list.value())
          std::println("domain: {}", name);
      } else if (std::holds_alternative<uint32_t>(uri->authority->host))
        std::println("host: {}", std::get<uint32_t>(uri->authority->host));
    }

    std::println("path: {}", uri->path);
  }
}
