// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Numeric suffix expander parsing example.

    This parses a string consisting of integers with suffixes. The suffixes denote a multiplier. The result of the parsing is
    an integer value that is the sum of each integer and the token multiplier.
*/

#include <unordered_map>
#include <cctype> // isdigit, isspace
#include <iostream> // debug (TODO: remove)

#include "swoc/TextView.h"
#include "swoc/swoc_file.h"
#include "catch.hpp"

using swoc::TextView;

// 100 M
// (potentially) modify the pass-in view, return the metric
inline TextView extractTailMetric(TextView &src) {
    src.rtrim_if(&isspace);
    auto const tail = src.back();
    if (isdigit(tail)) {
      // no metric specification, defaulting to the smallest unit
      return "B";
    } else {
      // take suffix from right to left -- from the first non-space char
      // (inclusive) to the first digit (exclusive)
      // can't call split/take_suffix_if because we don't want to discard any char
      // can't call suffix_if because it could be the beginning of the string
      auto const pos = src.rfind_if([](char const c){return isdigit(c) || c == ' ';});
      auto const metric = src.substr(pos + 1);
      src = src.prefix(pos + 1);
      return metric;
    }
}

inline uintmax_t extractTailMultiplier(TextView &src) {
    src.rtrim_if(&isspace);
    auto const pos = src.rfind_if([](char const c){return !isdigit(c);});
    if (pos == std::string_view::npos) {
      // no more non-digit on the left, this is the left-most multipler, parsing
      // is complete
      auto const multiplier = src;
      src.clear();
      return swoc::svtou(multiplier);
    } else {
      auto const multiplier = src.substr(pos);
      src = src.prefix(pos);
      return swoc::svtou(multiplier);
    }
}

/** Numeric suffix parser.
 *
 * Parse a string consisting of integers and suffixes, with each suffix representing a multiplier
 * for the preceding integer. The result is the sum of each integer and its multiplier.
 *
 * The suffixes can be optional or required. If optional, the lack of o suffix means a multiplier of
 * 1.
 *
 * For example, if the suffixes were "K" for 1024 and "M" for 1048576 then the string
 * "4M 384K" would parse to the value 4587520.
 *
 * The set of suffixes and their multipliers are local to an instance of the parser.
 */
class NumericSuffixParser {
  using self_type = NumericSuffixParser;
public:
  intmax_t operator()(TextView text) const;
protected:
  /// Suffix mapping, name -> multiplier.
  std::unordered_map<std::string, uintmax_t> _suffixes;
};

#define PRINT() \
    std::cout << "original: \"" << original << "\"\n"; \
    std::cout << "metric: \"" << extractTailMetric(original) << "\"\n"; \
    std::cout << "multiplier: \"" << extractTailMultiplier(original) << "\"\n"; \
    std::cout << "---- \n";

TEST_CASE("test TextView", "sanity") {
  SECTION("1 pair, no default") {
    std::vector<TextView> views = {
      "100M",
      " 100M",
      "100M ",
      " 100M ",
      "100 M",
      " 100 M",
      "100 M ",
      " 100 M ",
    };
    for (auto const view : views) {
      // make a copy
      auto original = view;
      CHECK("M" == extractTailMetric(original));
      CHECK(static_cast<uintmax_t>(100) == extractTailMultiplier(original));
    }
  }
  SECTION("1 pair, with default") {
    std::vector<TextView> views = {
      "100",
      " 100",
      "100 ",
      " 100 ",
      "100B",
      " 100B",
      "100B ",
      " 100B ",
      "100 B",
      " 100 B",
      "100 B ",
      " 100 B ",
    };
    for (auto const view : views) {
      // make a copy
      auto original = view;
      CHECK("B" == extractTailMetric(original));
      CHECK(static_cast<uintmax_t>(100) == extractTailMultiplier(original));
    }
  }
  SECTION("2 pair, no default") {
    std::vector<TextView> views = {
      "50G 100M",
    };
    for (auto const view : views) {
      // make a copy
      auto original = view;
      CHECK("M" == extractTailMetric(original));
      CHECK(static_cast<uintmax_t>(100) == extractTailMultiplier(original));
      CHECK("G" == extractTailMetric(original));
      CHECK(static_cast<uintmax_t>(50) == extractTailMultiplier(original));
    }
  }
}
