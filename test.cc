#include <print>

#include "uri.hh"

int
main(int argc, char** argv)
{
  if (argc < 2)
    exit(1);

  auto const uri = curi::URI::parse(argv[1]);
  if (not uri)
    std::println("failed {}", uri.error());
  else {
    std::println("{} {}", uri->scheme, uri->authority);
    if (uri->path)
      for (auto const& comp : *uri->path)
        std::println("{}", comp);
  }
}
