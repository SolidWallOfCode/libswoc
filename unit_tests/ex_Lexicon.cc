// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Lexicon example code.
*/

#include <bitset>

#include "swoc/Lexicon.h"
#include "swoc/swoc_file.h"
#include "swoc/swoc_ip.h"
#include "catch.hpp"

// Example code for documentatoin
// ---

// This is the set of address flags
// doc.1.begin
enum class NetType {
  EXTERNAL = 0, // 0x1
  PROD, // 0x2
  SECURE, // 0x4
  EDGE, // 0x8
  INVALID
};
// doc.1.end

// The number of distinct flags.
static constexpr size_t N_TYPES = size_t(NetType::INVALID);

// Set up a Lexicon to convert between the enumeration and strings.
// doc.2.begin
swoc::Lexicon<NetType> NetTypeNames { {{NetType::EXTERNAL, "external"},
                                       {NetType::PROD, "prod"},
                                       {NetType::SECURE, "secure"},
                                       {NetType::EDGE, "edge"}},
                                       NetType::INVALID // default value for undefined name
                                    };
// doc.2.end

// A bit set for the flags.
using Flags = std::bitset<N_TYPES>;

TEST_CASE("Lexicon Example", "[libts][Lexicon]") {
  swoc::IPSpace<Flags> space; // Space in which to store the flags.
  // Load the file contents
  swoc::TextView text { R"(
    10.0.0.2-10.0.0.254,edge
    10.12.0.0/25,prod
    10.15.0.10-10.15.0.99,prod,secure
    172.16.0.0/22,external,secure
    192.168.17.0/23,external,prod
  )" };
  // doc.load.begin
  // Process all the lines in the file.
  while (text) {
    auto line       = text.take_prefix_at('\n').trim_if(&isspace);
    auto addr_token = line.take_prefix_at(','); // first token is the range.
    swoc::IPRange r{addr_token};
    if (!r.empty()) { // empty means failed parse.
      Flags flags;
      while (line) { // parse out the rest of the comma separated elements
        auto token = line.take_prefix_at(',');
        auto idx   = NetTypeNames[token];
        if (idx != NetType::INVALID) { // one of the valid strings
          flags.set(idx); // set the bit
        }
      }
      space.mark(r, flags); // store the flags in the spae.
    }
  }
  // doc.load.end

  using AddrCase = std::tuple<swoc::IPAddr, Flags>;
  std::array<AddrCase, 5> AddrList = { {"10.0.0.6", 0x8} , {"172.19.20.31", 0x5}, {"192.168.18.19", 3}
  , {"10.15.0.57", 0x6}, {"10.12.0.126", 0x2}};

  for ( auto const& [ addr, bits ] : AddrList ) {
    // doc.lookup.begin
    auto && [ range, flags ] = space.find(addr);
    // doc.lookup.end
    REQUIRE(flags == bits);
  }
  // doc.lookup.end
}
