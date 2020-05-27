// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file

    Example tool to process network DB files.

    This generates an executable that processes a list of network files in a specific format.
    Tokens are (variable) space separated. Example lines look like

    4441:34F8:1E40:1EF:0:0:A0:0/108  partner:ngeo  raz     ?_  prod,dmz         "shared space"
    4441:34F8:1E40:1EF:0:0:B0:0/108  yahoo  raz     ?_  prod,dmz         "local routing"

    There is
    - an IP address network
    - type/owner field. If "yahoo", it's type "yahoo" and owner "yahoo". Otherwise it's a
      partner network, with "partner:" followed by the partner name.
    - pod / colo code
    - Useless column marker
    - Network property flags
    - Comment in double quotes

    The goal is to transform this in a standard CSV file, dropping the useless column, and
    tweaking the flags to use ';' instead of ',' as a separator (to avoid confusing the CSV).
    Adjacent networks with identical properties should also be merged to reduce the data set size.
    The comments may or not be kept - currently the implement discards them to get better
    network coalescence. One of the goals of the over all work is to make it easy to build
    variants of this code to output CSV files tuned to specific uses.
*/

#include <unordered_set>
#include <fstream>

#include "swoc/TextView.h"
#include "swoc/swoc_ip.h"
#include "swoc/bwf_ip.h"
#include "swoc/bwf_std.h"
#include "swoc/bwf_ex.h"
#include "swoc/swoc_file.h"
#include "swoc/Lexicon.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;
using swoc::IPEndpoint;

using swoc::IP4Addr;
using swoc::IP4Range;

using swoc::IP6Addr;
using swoc::IP6Range;

using swoc::IPAddr;
using swoc::IPRange;

using swoc::IPMask;

/// Type for temporary buffer writer output.
using W = swoc::LocalBufferWriter<512>;

/// Network properties.
enum class Flag {
  INVALID = -1, ///< Value for initialization, invalid.
  INTERNAL = 0, ///< Internal network.
  PROD, ///< Production network.
  DMZ, ///< DMZ
  SECURE, ///< Secure network
  NONE ///< No flags - useful because the input uses "-" sometimes to mark no properties.
};

namespace std {
/// Make the enum size look like a tuple size.
template <> struct tuple_size<Flag> : public std::integral_constant<size_t, static_cast<size_t>(Flag::NONE)> {};
} // namespace std

/// Bit set for network property flags.
using FlagSet = std::bitset<std::tuple_size<Flag>::value>;

/// Pod type.
enum class PodType {
  INVALID, ///< Initialization value.
  YAHOO, ///< Yahoo! pod.
  PARTNER ///< Partner pod.
};

/// Mapping of names and property flags.
swoc::Lexicon<Flag> FlagNames {{
                                   {Flag::NONE, {"-", "NONE"}}
                                   , {Flag::INTERNAL, { "internal" }}
                                   , {Flag::PROD, {"prod"}}
                                   , {Flag::DMZ, {"dmz"}}
                                   , {Flag::SECURE, {"secure"}}
                               }};

/// Mapping of names and pod types.
swoc::Lexicon<PodType> PodTypeNames {{
                                   {PodType::YAHOO, "yahoo"}
                                   , {PodType::PARTNER, "partner"}
                               }};

// Create BW formatters for the types so they can be used for output.
namespace swoc {

BufferWriter& bwformat(BufferWriter& w, bwf::Spec const& spec, PodType pt) {
  return bwformat(w, spec, (PodTypeNames[pt]));
}

BufferWriter& bwformat(BufferWriter& w, bwf::Spec const& spec, FlagSet const& flags) {
  bool first_p = true; // Track to get separators correct.
  // Loop through the indices and write the flag name if it's set.
  for ( unsigned idx = 0 ; idx < std::tuple_size<Flag>::value ; ++idx) {
    if (flags[idx]) {
      if (!first_p) {
        w.write(';');
      }
      bwformat(w, spec, FlagNames[static_cast<Flag>(idx)]);
      first_p = false;
    }
  }
  return w;
}
} // namespace SWOC_NAMESPACE

// These are used to keep pointers for the same string identical so the payloads
// can be directly compared.
std::unordered_set<TextView, std::hash<std::string_view>> PodNames;
std::unordered_set<TextView, std::hash<std::string_view>> OwnerNames;
std::unordered_set<TextView, std::hash<std::string_view>> Descriptions;

/// The "color" for the IPSpace.
struct Payload {
  PodType _type = PodType::INVALID; ///< Type of ownership.
  TextView _owner; ///< Corporate owner.
  TextView _pod; ///< Pod / colocation.
  TextView _descr; ///< Description / comment
  FlagSet _flags; ///< Flags.

  /// @return @c true if @a this is equal to @a that.
  bool operator == (Payload const& that) {
    return _type == that._type &&
    _owner == that._owner &&
    _pod == that._pod &&
    _flags == that._flags &&
    _descr == that._descr;
  }

  /// @return @c true if @a this is not equal to @a that.
  bool operator != (Payload const& that) {
    return ! (*this == that);
  }
};


/// IPSpace for mapping address to @c Payload
using Space = swoc::IPSpace<Payload>;

/// Place to store strings parsed from the input files.
swoc::MemArena Storage;

/// Convert a parsed string into a stored string to make the pointer persistent.
TextView store(TextView const& text) {
  auto span = Storage.alloc(text.size()).rebind<char>();
  memcpy(span, text);
  return span.view();
}

/// Process the @a content of a file in to @a space.
void process(Space& space, TextView content) {
  int line_no = 0; /// Track for error reporting.

  // For each line in @a content
  for  (TextView line ; ! (line = content.take_prefix_at('\n')).empty() ; ) {
    ++line_no;
    line.trim_if(&isspace);
    // Allow empty lines and '#' comments without error.
    if (line.empty() || '#' == *line) {
      continue;
    }

    // Get the range, make sure it's a valid range.
    auto range_token = line.take_prefix_if(&isspace);
    IPRange range{range_token};
    if (range.empty()) {
      std::cerr << W().print("Invalid range '{}' on line {}\n", range_token, line_no);
      continue;
    }

    // Get the owner / type.
    auto owner_token = line.ltrim_if(&isspace).take_prefix_if(&isspace);
    PodType pod_type = PodType::YAHOO; // default to this if no owner specifier found.
    if (auto type_token = owner_token.split_prefix_at(':') ; ! type_token.empty()) {
      pod_type = PodTypeNames[type_token];
      if (PodType::INVALID == pod_type) {
        std::cerr << W().print("Invalid type '{}' on line {}\n", type_token, line_no);
        continue;
      }
    }

    // normalize owner
    if ( auto spot = OwnerNames.find(owner_token) ; spot == OwnerNames.end()) {
      owner_token = store(owner_token);
      OwnerNames.insert(owner_token);
    } else {
      owner_token = *spot;
    }

    // Get the pod code.
    auto pod_token = line.ltrim_if(&isspace).take_prefix_if(&isspace);

    // normalize
    if ( auto spot = PodNames.find(pod_token) ; spot == PodNames.end()) {
      pod_token = store(pod_token);
      PodNames.insert(pod_token);
    } else {
      pod_token = *spot;
    }

    line.ltrim_if(&isspace).take_prefix_if(&isspace); // skip bogus column.

    // Work on the flags.
    auto flag_token = line.ltrim_if(&isspace).take_prefix_if(&isspace);
    FlagSet flags;
    // Loop over the token, picking out comma separated keys.
    for ( TextView key ; ! (key = flag_token.take_prefix_at(',')).empty() ; ) {
      auto idx = FlagNames[key]; // look up the key.
      if (Flag::INVALID == idx) {
        std::cerr << W().print("Invalid flag '{}' on line {}\n", key, line_no);
        continue;
      }
      if (idx != Flag::NONE) { // "NONE" means the input was marked explicitly as no flags.
        flags[int(idx)] = true;
      }
    }

    #if 0
    // The description is what's left, trim the spaces and then the quotes.
    auto descr_token = line.trim_if(&isspace).trim('"');
    // normalize
    if ( auto spot = Descriptions.find(descr_token) ; spot == Descriptions.end()) {
      descr_token = store(descr_token);
      Descriptions.insert(descr_token);
    } else {
      descr_token = *spot;
    }
    #endif

    // Everything went OK, create the payload and put it in the space.
    Payload payload{pod_type, owner_token, pod_token, {}, flags};
    space.blend(range, payload, [&](Payload & lhs, Payload const& rhs) {
      // It's an error if there's overlap that's not consistent.
      if (lhs._type != PodType::INVALID && lhs != rhs) {
        std::cerr << W().print("Range collision while blending {} on line {}\n", range, line_no);
      }
      lhs = rhs;
      return true;
    });
  }
}

void post_processing_performance_test(Space & old_space) {
  Space space;

  swoc::file::path vz_db_path{"vz_netdb.csv"};
  std::error_code ec;
  auto t0 = std::chrono::system_clock::now();
  std::string content = swoc::file::load(vz_db_path, ec);

  TextView text{content};
  unsigned line_no = 0;
  for  (TextView line ; ! (line = text.take_prefix_at('\n')).empty() ; ) {
    ++line_no;
    // Get the range, make sure it's a valid range.
    auto range_token = line.take_prefix_at(',');
    IPRange range{range_token};

    // Get the owner / type.
    auto pod_type = PodTypeNames[line.take_prefix_at(',')];
    auto owner = line.take_prefix_at(',');
    auto pod_token = line.take_prefix_at(',');
    auto flag_token = line.take_prefix_at(',');
    FlagSet flags;
    // Loop over the token, picking out keys
    for ( TextView key ; ! (key = flag_token.take_prefix_at(';')).empty() ; ) {
      auto idx = FlagNames[key]; // look up the key.
      if (Flag::INVALID == idx) {
        std::cerr << W().print("Invalid flag '{}'\n", key);
        continue;
      }
      flags[int(idx)] = true;
    }

    // Everything went OK, create the payload and put it in the space.
    Payload payload{pod_type, owner, pod_token, {}, flags};
    space.mark(range, payload);
  }
  auto vz_delta = std::chrono::system_clock::now() - t0;
  std::cout << W().print("Reload time - {} ms\n",
      std::chrono::duration_cast<std::chrono::milliseconds>(vz_delta).count());
  if (line_no != space.count()) {
    std::cerr << W().print("Space count {} does not match line count {}\n", space.count(), line_no);
  }

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
  t0 = std::chrono::system_clock::now();
  for ( auto const& addr : a4) {
    [[maybe_unused]] auto spot = space.find(addr);
  }
  vz_delta = std::chrono::system_clock::now() - t0;
  std::cout << W().print("IPv4 time - {} addresses, {} ns total, {} ns per lookup\n",
    a4.size(), vz_delta.count(), vz_delta.count() / a4.size());

  t0 = std::chrono::system_clock::now();
  for ( auto const& addr : a6) {
    [[maybe_unused]] auto spot = space.find(addr);
  }
  vz_delta = std::chrono::system_clock::now() - t0;
  std::cout << W().print("IPv6 time - {} addresses, {} ns total, {} ns per lookup\n",
      a6.size(), vz_delta.count(), vz_delta.count() / a6.size());

  t0 = std::chrono::system_clock::now();
  for ( auto const& addr : a4) {
    [[maybe_unused]] auto spot = old_space.find(addr);
  }
  vz_delta = std::chrono::system_clock::now() - t0;
  std::cout << W().print("IPv4 time (pre-cleaning) - {} addresses, {} ns total, {} ns per lookup\n",
      a4.size(), vz_delta.count(), vz_delta.count() / a4.size());

  t0 = std::chrono::system_clock::now();
  for ( auto const& addr : a6) {
    [[maybe_unused]] auto spot = old_space.find(addr);
  }
  vz_delta = std::chrono::system_clock::now() - t0;
  std::cout << W().print("IPv6 time (pre-cleaning) - {} addresses, {} ns total, {} ns per lookup\n",
      a6.size(), vz_delta.count(), vz_delta.count() / a6.size());
}

int main(int argc, char *argv[]) {
  Space space;

  // Set the defaults so bogus input doesn't throw.
  PodTypeNames.set_default(PodType::INVALID);
  FlagNames.set_default(Flag::INVALID);

  // Open the output file.
  std::ofstream output;
  output.open("vz_netdb.csv", std::ofstream::out | std::ofstream::trunc);
  if (! output.is_open()) {
    std::cerr << W().print("Unable to open output file: {}", swoc::bwf::Errno{errno});
  }

  auto t0 = std::chrono::system_clock::now();
  // Process the files in the command line.
  for ( int idx = 1 ; idx < argc ; ++idx ) {
    swoc::file::path path(argv[idx]);
    std::error_code ec;
    std::string content = swoc::file::load(path, ec);
    if (! ec) {
      std::cout << W().print("Processing {}, {} bytes\n", path, content.size());
      process(space, content);
    }
  }
  auto read_delta = std::chrono::system_clock::now() - t0;

  // Dump the resulting space.
  std::cout << W().print("{} ranges\n", space.count());
  for ( auto && [ r, p] : space) {
    // Note - if description is to be used, it needs to be added here.
    output << W().print("{},{},{},{},{}\n", r, p._type, p._owner, p._pod, p._flags);
  }

  auto write_delta = std::chrono::system_clock::now() - t0;

  std::cout << W().print("Read & process time - {} ms, write time {} ms\n",
    std::chrono::duration_cast<std::chrono::milliseconds>(read_delta).count(),
    std::chrono::duration_cast<std::chrono::milliseconds>(write_delta - read_delta).count());

  post_processing_performance_test(space);
  return 0;
}
