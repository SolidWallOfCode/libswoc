// SPDX-License-Identifier: Apache-2.0
// Copyright 2022 Network Geographics

/** @file

    Using Lexicon to represent a UNIX host file.
*/

#include <ios>

#include "swoc/TextView.h"
#include "swoc/swoc_ip.h"
#include "swoc/Lexicon.h"
#include "swoc/bwf_ip.h"
#include "swoc/bwf_ex.h"
#include "swoc/bwf_std.h"
#include "swoc/swoc_file.h"
#include "swoc/Errata.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;
using swoc::Errata;
using swoc::IPAddr;
using swoc::IP4Addr;
using swoc::IP6Addr;

using V4Lexicon = swoc::Lexicon<IP4Addr>;
using V6Lexicon = swoc::Lexicon<IP6Addr>;

// --------------------------------------------------

static constexpr TextView HOST_FILE { R"(
127.0.0.1   localhost localhost.localdomain localhost4 localhost4.localdomain4
::1         localhost localhost.localdomain localhost6 localhost6.localdomain6

192.168.56.233	tiphares
192.168.56.97	spira
192.168.3.22	livm
192.168.2.12	atc-build

192.168.2.2	ns1 ns1.cdn.swoc.io
192.168.2.3	ns2 ns2.cdn.swoc.io
192.168.2.4	atc-dns dns.cdn.swoc.io
192.168.2.10	atc-ops
192.168.2.11	atc-portal
192.168.2.33	atc-monitor atc-mon

192.168.2.19	mid-ts
192.168.2.32	edge-ts
)"};

int main(int, char *[]) {
  V4Lexicon hosts_ipv4;
  V6Lexicon hosts_ipv6;

  TextView src{HOST_FILE};
  while (src) {
    auto line = src.take_prefix_at('\n').ltrim_if(&isspace);
    if (!line || *line == '#') {
      continue;
    }
    auto addr_token = line.take_prefix_if(&isspace);
    IPAddr addr;
    if (! addr.load(addr_token)) {
      continue; // invalid address.
    }
    while (line.ltrim_if(&isspace)) {
      auto host = line.take_prefix_if(&isspace);
      if (addr.is_ip4()) {
        hosts_ipv4.define(IP4Addr(addr), host);
      } else if (addr.is_ip6()) {
        hosts_ipv6.define(IP6Addr(addr), host);
      }
    }
  }

  std::cout << swoc::detail::what("{} -> {}\n", "ns2.cdn.swoc.io", hosts_ipv4["ns2.cdn.swoc.io"]);
  std::cout << swoc::detail::what("{} -> {}\n", "ns2", hosts_ipv4["ns2"]);
  std::cout << swoc::detail::what("{} -> {}\n", IP4Addr("192.168.2.3"), hosts_ipv4[IP4Addr("192.168.2.3")]);

  std::cout << "Table dump by name" << std::endl;
  for ( auto const & item : hosts_ipv4.by_names()) {
    std::cout << swoc::detail::what("{} -> {}\n", std::get<V4Lexicon::NAME_IDX>(item), std::get<V4Lexicon::VALUE_IDX>(item));
  }
  for ( auto const & item : hosts_ipv6.by_names()) {
    std::cout << swoc::detail::what("{} -> {}\n", std::get<V4Lexicon::NAME_IDX>(item), std::get<V4Lexicon::VALUE_IDX>(item));
  }

  return 0;
}
