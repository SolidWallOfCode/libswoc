// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Verizon Media

/** @file

    Example of a variant of IPSpace optimized for fast loading.

    This will build the flat files if given the --build option.

    This will look up addresses from the flat files given the --find option.

    Build flat files from "data.csv"
    --build data.csv

    Lookup some addresses.
    --find 172.17.18.19 2001:BADF::0E0E

    Build and lookup
    --build data.csv --find 172.17.18.19 2001:BADF::0E0E
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

#include "swoc/TextView.h"
#include "swoc/swoc_ip.h"
#include "swoc/bwf_ip.h"
#include "swoc/bwf_ex.h"
#include "swoc/bwf_std.h"
#include "swoc/swoc_file.h"
#include "swoc/Errata.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;
using swoc::MemSpan;
using swoc::Errata;

using swoc::IPRange;
using swoc::IPAddr;
using swoc::IP4Addr;
using swoc::IP6Addr;
using swoc::IPSpace;

// Temp for error messages.
std::string err_text;

std::error_code errno_code() { return std::error_code(errno, std::system_category()); }

/** Allocate a span of type @a T.
 *
 * @tparam T Element type.
 * @param n Number of elements.
 * @return A @c MemSpan<T> with @a n elements.
 */
template < typename T > MemSpan<T> span_alloc(size_t n) {
  size_t bytes = sizeof(T) * n;
  return { static_cast<T*>(malloc(bytes)), n };
}

// Array type for flat files.
// Each flat file is an instance of this, which is a wrapper over an array of its nodes.
template < typename METRIC, typename PAYLOAD > class IPArray {
public:
  struct Node {
    METRIC _min;
    METRIC _max;
    PAYLOAD _payload;
  };

  /** Load from array already in memory.
   *
   * @param span Memory containing the array.
   *
   * The presumption is @a span has been brough in to memory from a file via @c mmap.
   */
  IPArray(MemSpan<Node> span) : _nodes(span) {}

  /** Load from a @a space.
   *
   * @param space Populated IPSpace.
   *
   * This will allocate memory to hold an array that mirrors the data in @a space.
   */
  IPArray(IPSpace<PAYLOAD> const& space);

  /** Find @a addr.
   *
   * @param addr Search value.
   * @return The node that contains @a addr, or @c nullptr if not found.
   */
  Node* find(METRIC const& addr);

  Errata store(swoc::file::path const& path);

  size_t size() const { return _nodes.size(); }
  void* data() const { return _nodes.data(); }

protected:
  /// Array memory.
  MemSpan<Node> _nodes;
};

// Standard binary search on a sorted array.
template < typename METRIC, typename PAYLOAD > auto IPArray<METRIC, PAYLOAD>::find(METRIC const& addr) -> Node* {
  ssize_t lidx = 0;
  ssize_t ridx = _nodes.count() - 1;

  while (lidx <= ridx) { // still some array left to search.
    auto idx = (lidx + ridx) / 2; // Look at the middle element.
    auto n = _nodes.data() + idx;
    if (addr < n->_min) { // target is to the left
      ridx = idx - 1;
    } else if (addr > n->_max) { // target is to the right
      lidx = idx + 1;
    } else { // target is right here, done.
      return n;
    }
  }
  return nullptr; // dropped out of the loop -> not found.
}

template < typename METRIC, typename PAYLOAD > IPArray<METRIC, PAYLOAD>::IPArray(IPSpace<PAYLOAD> const& space) {
  auto n = space.count(METRIC::AF_value);
  _nodes = { span_alloc<Node>(n) }; // In memory array.
  auto node = _nodes.data();
  // Update the array with all IPv4 ranges.
  for ( auto spot = space.begin(METRIC::AF_value), limit = space.end(METRIC::AF_value) ; spot != limit ; ++spot, ++node ) {
    auto && [ range, payload ] { *spot };
    node->_min = static_cast<METRIC>(range.min());
    node->_max = static_cast<METRIC>(range.max());
    node->_payload = payload;
  }

}

template <typename METRIC, typename PAYLOAD> Errata IPArray<METRIC, PAYLOAD>::store(swoc::file::path const &path) {
  auto fd = ::open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd >= 0) {
    auto written = ::write(fd, _nodes.data(), _nodes.size());
    if (written != ssize_t(_nodes.size())) {
      return Errata(errno_code(), "Failed to write IP4 output - {} of {} bytes written to '{}'\n", written, _nodes.size(), path);
    }
    close(fd);
  } else {
    return Errata(errno_code(), "Failed to open IP4 output '{}'\n", path);
  }
  return {};
}

using A4 = IPArray<IP4Addr, unsigned>;
using A6 = IPArray<IP6Addr, unsigned>;

// Load the CSV file @a src into @a space.
void build(IPSpace<unsigned> & space, swoc::file::path src) {
  std::error_code ec;
  auto content = swoc::file::load(src, ec);
  TextView text { content };
  while (text) {
    auto line = text.take_prefix_at('\n');
    if ('#' == *line) {
      continue;
    }
    auto addr_token = line.take_prefix_at(',');
    IPRange range{addr_token};
    space.mark(range, swoc::svtou(line));
  }
}

int main(int argc, char const *argv[]) {
  swoc::file::path path_4{"/opt/ip4.mem"};
  swoc::file::path path_6{"/tmp/ip6.mem"};
  swoc::file::path src;

  MemSpan<char const*> args{argv, size_t(argc)};
  args.remove_prefix(1); // drop executable name.
  if (args.empty()) {
    exit(0); // nothing to do.
  }

  // Check if the array files need to be built.
  if (0 == strcasecmp("--build"_tv, args.front())) {
    IPSpace<unsigned int> space;
    args.remove_prefix(1);
    while (!args.empty() && ! TextView(std::string_view(args[0])).starts_with("-")) {
      build(space, swoc::file::path(args[0]));
      args.remove_prefix(1);
    }
    if ( auto errata = A4(space).store(path_4) ; !errata.is_ok() ) {
      std::cerr << errata << std::endl;
      exit(1);
    }
    if (auto errata = A6(space).store(path_6) ; !errata.is_ok()) {
      std::cerr << errata << std::endl;
      exit(1);
    }
  }

  if (args.empty()) {
    exit(0);
  }

  if (0 == strcasecmp("--find"_tv, args.front())) {
    args.remove_prefix(1);
  } else {
    std::cerr << swoc::bwprint(err_text, "Unrecognized argument '{}'\n", args.front());
    exit(1);
  }

  std::error_code ec;

  auto t0 = std::chrono::system_clock::now();
  // Make sure the flat files are there.
  auto stat_4 = swoc::file::status(path_4, ec);
  if (ec) {
    std::cerr << swoc::bwprint(err_text, "Flat file for IPv4 '{}' not found. {}\n", path_4, ec);
    exit(1);
  }

  auto stat_6 = swoc::file::status(path_6, ec);
  if (ec) {
    std::cerr << swoc::bwprint(err_text, "Flat file for IPv6 '{}' not found. {}\n", path_6, ec);
    exit(1);
  }

  // map the flat files in to memory.
  auto fsize = swoc::file::file_size(stat_4);
  auto fd_4 = ::open(path_4.c_str(), O_RDONLY);
  MemSpan<void> mem4 { ::mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd_4, 0), size_t(fsize) };
  A4 a_4 { mem4.rebind<A4::Node>() };

  fsize = swoc::file::file_size(stat_6);
  auto fd_6 = ::open(path_6.c_str(), O_RDONLY);
  MemSpan<void> mem6 { ::mmap(nullptr, fsize, PROT_READ, MAP_PRIVATE, fd_6, 0), size_t(fsize) };
  A6 a_6 { mem6.rebind<A6::Node>() };

  std::cout << swoc::bwprint(err_text, "Mapped files in {} us\n", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - t0).count());

  #if 0
  // performance testing.
  t0 = std::chrono::system_clock::now();
  auto step = ~0U / 10000000;
  IP4Addr addr {in_addr_t(1)};
  for ( unsigned idx = 0 ; idx < 10000000 ; ++idx ) {
    [[maybe_unused]] auto n = a_4.find(addr);
    addr = addr.host_order() + step;
  }
  auto delta = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - t0);
  std::cout << swoc::bwprint(err_text, "Searched files in {} ns - {} ns / lookup\n", delta.count(), delta.count() / 10000000);
  #endif

  // Now the in memory flat files can be searched.
  while (! args.empty()) {
    IPAddr addr;
    if (addr.load(args.front())) {
      if (addr.is_ip4()) {
        auto n = a_4.find(addr.ip4());
        if (n) {
          std::cout << swoc::bwprint(err_text, "{} -> {}\n", addr, n->_payload);
        } else {
          std::cout << swoc::bwprint(err_text, "{} not found\n", addr);
        }
      } else if (addr.is_ip6()) {
        auto n = a_6.find(addr.ip6());
        if (n) {
          std::cout << swoc::bwprint(err_text, "{} -> {}\n", addr, n->_payload);
        } else {
          std::cout << swoc::bwprint(err_text, "{} not found\n", addr);
        }
      }

    } else {
      std::cerr << swoc::bwprint(err_text, "Unrecognized address '{}'\n", args.front());
    }
    args.remove_prefix(1);
  }
  return 0;
}
