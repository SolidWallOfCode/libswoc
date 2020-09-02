// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Numeric suffix expander parsing example.

    This parses a string consisting of integers with suffixes. The suffixes denote a multiplier. The result of the parsing is
    an integer value that is the sum of each integer and the token multiplier.
*/

#include <unordered_map>
#include <cctype> // isdigit, isspace, tolower
#include <algorithm> // std::equal
#include <iostream> // debug (TODO: remove)

#include "swoc/TextView.h"
#include "swoc/swoc_file.h"
#include "catch.hpp"

using swoc::TextView;

// The Metric concept
// A metric is a measurement system which can be computer storage, time duration
// etc.
/*
struct Metric {
  // the base (smallest) unit of the metric system
  static constexpr TextView baseUnit = "";

  // convert different units to a multiple to base unit
  static inline uintmax_t convertToBase(TextView const unit);

  // canonicalize the spelling of the metric. e.g. both "b" and "byte" mean the
  // same thing
  // intentionally *not* taking a const unit b/c there might be use cases where
  // the implementation wants to modify the view and then return it
  static inline TextView canonicalize(TextView unit);
};
*/

inline bool iequal(TextView const lhs, TextView const rhs) noexcept {
  struct Pred {
    bool operator()(char const l, char const r) noexcept {
      return tolower(l) == tolower(r);
    }
  };
  return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), Pred{});
}

// for classes that meet the requirements of the Metric concept
namespace metric {
struct Storage {
  static constexpr TextView B = "B";
  static constexpr TextView KB = "KB";
  static constexpr TextView MB = "MB";
  static constexpr TextView GB = "GB";
  static constexpr TextView TB = "TB";
  static constexpr TextView PB = "PB";

  static constexpr TextView baseUnit = B;

  static uintmax_t convertToBase(TextView const unit) {
    if (unit == B) {
      return static_cast<uintmax_t>(1);
    } else if (unit == KB) {
      return static_cast<uintmax_t>(1) << 10;
    } else if (unit == MB) {
      return static_cast<uintmax_t>(1) << 20;
    } else if (unit == GB) {
      return static_cast<uintmax_t>(1) << 30;
    } else if (unit == TB) {
      return static_cast<uintmax_t>(1) << 40;
    } else if (unit == PB) {
      return static_cast<uintmax_t>(1) << 50;
    } else {
      // TODO: error handling
      return static_cast<uintmax_t>(0);
    }
  }

  static inline TextView canonicalize(TextView unit) {
    if (iequal(unit, "b") || iequal(unit, "byte")) {
      return B;
    } else if (iequal(unit, "k") || iequal(unit, "kb")) {
      return KB;
    } else if (iequal(unit, "m") || iequal(unit, "mb")) {
      return MB;
    } else if (iequal(unit, "g") || iequal(unit, "gb")) {
      return GB;
    } else if (iequal(unit, "t") || iequal(unit, "tb")) {
      return TB;
    } else if (iequal(unit, "p") || iequal(unit, "pb")) {
      return PB;
    } else {
      // TODO: error handling
      return B;
    }
  }
};

struct Duration {
  static constexpr TextView SECOND = "second";
  static constexpr TextView MINUTE = "minute";
  static constexpr TextView HOUR = "hour";
  static constexpr TextView DAY = "day";
  static constexpr TextView WEEK = "week";

  static constexpr TextView baseUnit = "second";
};

}  // namespace metric

// (potentially) modifies the pass-in view. returns the unit
template <typename Metric>
inline TextView rExtractUnit(TextView &src) noexcept {
    src.rtrim_if(&isspace);
    if (isdigit(src.back())) {
      // no unit specification, defaulting to the smallest unit
      return Metric::baseUnit;
    } else {
      // take suffix from right to left -- from the first non-space char
      // (inclusive) to the first digit (exclusive)
      // can't call split/take_suffix_if because we don't want to discard any char
      // can't call suffix_if because it could be the beginning of the string
      auto const pos = src.rfind_if([](char const c){return isdigit(c) || c == ' ';});
      auto const unit = src.substr(pos + 1);
      src = src.prefix(pos + 1);
      return unit;
    }
}

// (potentially) modifies the pass-in view. returns the multiplier
inline uintmax_t rExtractMultiplier(TextView &src) {
    src.rtrim_if(&isspace);
    auto const pos = src.rfind_if([](char const c){return !isdigit(c);});
    if (pos == std::string_view::npos) {
      // no more non-digit on the left which means this is the left-most
      // multipler. parsing is complete
      auto const multiplier = src;
      src.clear();
      return swoc::svtou(multiplier);
    } else {
      auto const multiplier = src.substr(pos + 1);
      src = src.prefix(pos + 1);
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
template <typename Metric>
class NumericSuffixParser {
  using self_type = NumericSuffixParser;
public:
  intmax_t operator()(TextView text) {
    parseSuffixes(text);
    return sumUp();
  }
  void clear() {
    _suffixes.clear();
  }
protected:
  /// Suffix mapping, name -> multiplier.
  std::unordered_map<std::string, uintmax_t> _suffixes;

private:
  friend class SuffixExpanderTestFixture; 
  uintmax_t sumUp() const {
    uintmax_t res = 0;
    for (auto const &tuple : _suffixes) {
      res += Metric::convertToBase(tuple.first) * (tuple.second);
    }
    return res;
  }

  void parseSuffixes(TextView text) {
    clear();
    while (text.rtrim_if(&isspace)) {
      auto const unit = Metric::canonicalize(rExtractUnit<Metric>(text));
      if (text.rtrim_if(&isspace).empty()) {
        // TODO: error handling: missing multipler for a unit
      }
      auto const multiplier = rExtractMultiplier(text);
      // no heterogeneous lookup for std::unoredered_map until C++20. had to
      // construct the key type explicitly
      auto const [it, inserted] = _suffixes.try_emplace(
          std::string(unit), multiplier);
      if (!inserted) {
        it->second += multiplier;
      }
    }
  }
};

// provide access to private members of NumericSuffixParser<>
class SuffixExpanderTestFixture {
};

TEST_CASE("NumericSuffixParser free helper function test", "[libswoc][example][NumericSuffixParser][free helper functions]") {
  SECTION("iequal", "positive") {
    CHECK(iequal("a", "A"));
    CHECK(iequal("a", "a"));
    CHECK(iequal("A", "a"));

    CHECK(iequal("ab", "ab"));
    CHECK(iequal("ab", "Ab"));
    CHECK(iequal("ab", "aB"));
    CHECK(iequal("ab", "AB"));

    CHECK(iequal("Ab", "ab"));
    CHECK(iequal("Ab", "aB"));
    CHECK(iequal("Ab", "Ab"));
    CHECK(iequal("Ab", "AB"));

    CHECK(iequal("aB", "ab"));
    CHECK(iequal("aB", "Ab"));
    CHECK(iequal("aB", "aB"));
    CHECK(iequal("aB", "AB"));

    CHECK(iequal("AB", "ab"));
    CHECK(iequal("AB", "Ab"));
    CHECK(iequal("AB", "aB"));
    CHECK(iequal("AB", "AB"));
  }

  SECTION("iequal", "negative") {
    CHECK(!iequal("a", ""));
    CHECK(!iequal("a", "b"));
    CHECK(!iequal("a", "aB"));

    CHECK(!iequal("", "a"));
    CHECK(!iequal("", "B"));
    CHECK(!iequal("", "aB"));
  }
}

TEST_CASE("NumericSuffixParser parsing algorithm", "[libswoc][example][NumericSuffixParser][parsing]") {
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
      CHECK("M" == rExtractUnit<metric::Storage>(original));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(original));
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
      CHECK("B" == rExtractUnit<metric::Storage>(original));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(original));
    }
  }
  SECTION("2 pairs, no default") {
    std::vector<TextView> views = {
      "50G100M",
      " 50G100M",
      "50G100M ",
      " 50G100M ",

      "50G 100M",
      " 50G 100M",
      "50G 100M ",
      " 50G 100M ",

      "50 G 100M",
      " 50 G 100M",
      "50 G 100M ",
      " 50 G 100M ",

      "50 G100M",
      " 50 G100M",
      "50 G100M ",
      " 50 G100M ",

      "50G 100 M",
      " 50G 100 M",
      "50G 100 M ",
      " 50G 100 M ",

      "50G100 M",
      " 50G100 M",
      "50G100 M ",
      " 50G100 M ",

      "50 G 100 M",
      " 50 G 100 M",
      "50 G 100 M ",
      " 50 G 100 M ",

      "50 G100 M",
      " 50 G100 M",
      "50 G100 M ",
      " 50 G100 M ",
    };
    for (auto const view : views) {
      // make a copy
      INFO("src=\"" << view << "\"");
      auto src = view;
      CHECK("M" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(src));
      CHECK("G" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(50) == rExtractMultiplier(src));
      src.rtrim_if(&isspace);
      CHECK(src.empty());
    }
  }
  SECTION("2 pairs, with defaults on the right") {
    std::vector<TextView> views = {
      "50G100",
      " 50G100",
      "50G100 ",
      " 50G100 ",

      "50G 100",
      " 50G 100",
      "50G 100 ",
      " 50G 100 ",

      "50 G 100",
      " 50 G 100",
      "50 G 100 ",
      " 50 G 100 ",

      "50 G100",
      " 50 G100",
      "50 G100 ",
      " 50 G100 ",
    };
    for (auto const view : views) {
      // make a copy
      INFO("src=\"" << view << "\"");
      auto src = view;
      CHECK("B" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(src));
      CHECK("G" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(50) == rExtractMultiplier(src));
      src.rtrim_if(&isspace);
      CHECK(src.empty());
    }
  }
  SECTION("2 pairs, with defaults on the left") {
    std::vector<TextView> views = {
      "50 100M",
      " 50 100M",
      "50 100M ",
      " 50 100M ",

      "50 100 M",
      " 50 100 M",
      "50 100 M ",
      " 50 100 M ",
    };
    for (auto const view : views) {
      // make a copy
      INFO("src=\"" << view << "\"");
      auto src = view;
      CHECK("M" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(src));
      CHECK("B" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(50) == rExtractMultiplier(src));
      src.rtrim_if(&isspace);
      CHECK(src.empty());
    }
  }
  SECTION("2 pairs, both are defaults") {
    std::vector<TextView> views = {
      "50 100",
      " 50 100",
      "50 100 ",
      " 50 100 ",
    };
    for (auto const view : views) {
      // make a copy
      INFO("src=\"" << view << "\"");
      auto src = view;
      CHECK("B" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(src));
      CHECK("B" == rExtractUnit<metric::Storage>(src));
      CHECK(static_cast<uintmax_t>(50) == rExtractMultiplier(src));
      src.rtrim_if(&isspace);
      CHECK(src.empty());
    }
  }
}

TEST_CASE_METHOD(SuffixExpanderTestFixture, "parseSuffixes() test") {
  TextView text = "100 KB50G 50 K 20";
  NumericSuffixParser<metric::Storage> parser;
  CHECK(parser(text) == static_cast<uintmax_t>(100L * (1 << 10) + 50L * (1 << 30) + 50L * (1 << 10) + 20L));
}
