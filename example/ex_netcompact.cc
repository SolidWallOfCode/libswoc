// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file

    Example tool to compact networks.

    Input is a file with addresses. Each line should be an address, an address range,
    or a network in CIDR format.

    4441:34F8:1E40:1EF:0:0:A0:0/108
    192.168.12.1
    10.0.23.0-10.0.14.255

    The output is the same set of addresses in as few networks as possible.
*/

#include <unordered_set>
#include <fstream>

#include "swoc/TextView.h"
#include "swoc/swoc_ip.h"
#include "swoc/bwf_ip.h"
#include "swoc/bwf_std.h"
#include "swoc/bwf_std.h"
#include "swoc/swoc_file.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;

using swoc::IPRange;

/// Type for temporary buffer writer output.
using W = swoc::LocalBufferWriter<512>;

/// IPSpace for mapping address. Treating it as a set so use a no-data payload.
using Space = swoc::IPSpace<std::monostate>;

void post_processing_performance_test(Space & space);

/// Process the @a content of a file in to @a space.
unsigned process(Space& space, TextView content) {
  int line_no = 0; /// Track for error reporting.
  unsigned n_ranges = 0;

  // For each line in @a content
  while (content) {
    TextView line = content.take_prefix_at('\n').trim_if(&isspace);
    ++line_no;
    // Allow empty lines and '#' comments without error.
    if (line.empty() || '#' == *line) {
      continue;
    }

    // Get the range, make sure it's a valid range.
    IPRange range{line};
    if (range.empty()) {
      std::cerr << W().print("Invalid range '{}' on line {}\n", line, line_no);
      continue;
    }
    ++n_ranges;
    space.mark(range, std::monostate{});
  }
  return n_ranges;
}

int main(int argc, char *argv[]) {
  Space space;

  if (argc < 2) {
    std::cerr << W().print("Input file name required.\n");
    exit(1);
  }

  auto t0 = std::chrono::system_clock::now(); // timing
  // Load the file.
  swoc::file::path path{argv[1]};
  std::error_code ec;
  std::string content = swoc::file::load(path, ec);
  if (ec) {
    std::cerr << W().print(R"(Failed to open file "{}" - {}\n)", path, ec);
    exit(1);
  }
  // Paint the IPSpace.
  auto n_ranges = process(space, content);

  // Dump the results.
  unsigned n_nets = 0;
  for ( auto && [range, payload] : space ) {
    for ( auto && net : range.networks() ) {
      ++n_nets;
      std::cout << W().print("{}\n", net);
    }
  }

  auto delta = std::chrono::system_clock::now() - t0;

  std::cerr << W().print("{} ranges in, {} ranges condensed, {} networks out in {} ms\n"
    , n_ranges, space.count(), n_nets
    , std::chrono::duration_cast<std::chrono::milliseconds>(delta).count());

  post_processing_performance_test(space);
  return 0;
}

void post_processing_performance_test(Space & space) {
  using swoc::IP4Addr;
  using swoc::IP6Addr;

  std::vector<IP4Addr> a4;
  std::vector<IP6Addr> a6;
  for ( auto && [ r, p] : space) {
    if (r.is_ip4()) {
      IP4Addr a = r.min().ip4();
      a4.push_back(a);
      a4.push_back(--IP4Addr(a));
      a4.push_back(++IP4Addr(a));
      a = r.max().ip4();
      a4.push_back(a);
      a4.push_back(--IP4Addr(a));
      a4.push_back(++IP4Addr(a));
    } else if (r.is_ip6()) {
      IP6Addr a = r.min().ip6();
      a6.push_back(a);
      a6.push_back(--IP6Addr(a));
      a6.push_back(++IP6Addr(a));
      a = r.max().ip6();
      a6.push_back(a);
      a6.push_back(--IP6Addr(a));
      a6.push_back(++IP6Addr(a));
    }
  }

  if (! a4.empty()) {
    auto t0 = std::chrono::system_clock::now();
    for (auto const &addr : a4) {
      [[maybe_unused]] auto spot = space.find(addr);
    }
    auto delta = std::chrono::system_clock::now() - t0;
    std::cout << W().print("IPv4 time - {} addresses, {} ns total, {} ns per lookup\n", a4.size(), delta.count(),
                           delta.count() / a4.size());
  }

  if (!a6.empty()) {
    auto t0 = std::chrono::system_clock::now();
    for (auto const &addr : a6) {
      [[maybe_unused]] auto spot = space.find(addr);
    }
    auto delta = std::chrono::system_clock::now() - t0;
    std::cout << W().print("IPv6 time - {} addresses, {} ns total, {} ns per lookup\n", a6.size(), delta.count(),
                           delta.count() / a6.size());
  }
}
