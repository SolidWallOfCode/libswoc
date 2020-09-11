// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Numeric suffix expander parsing example.

    This parses a string consisting of integers with suffixes. The suffixes denote a multiplier. The result of the parsing is
    an integer value that is the sum of each integer and the token multiplier.
*/

#include <unordered_map>
#include <cctype>    // isdigit, isspace, tolower
#include <algorithm> // std::equal
#include <stdexcept> // std::runtime_error

#include "swoc/TextView.h"
#include "swoc/swoc_file.h"
#include "catch.hpp"

using swoc::TextView;

// homework assumptions:
// 1. the NumericSuffixParser class always clears its states automatically when
//    parsing a (new) string
// 2. the input string has alternating patterns and numeric multipliers and
//    units
// 3. there can be zero or more spaces between a multiplier and its unit;
//    or between a unit and the multiplier (belonging to a different pair) on
//    the right
// 4. there cannot be any space that breaks up a single multiplier or a unit
// 5. at compile time (by the time the writer writes the parsing code), s/he
//    already knows what kind of metric system (time duration, computer storage
//    etc.) that s/he is parsing

// The Metric concept
// A metric is a measurement system which can be computer storage, time duration
// etc.
/*
struct Metric {
  // the base (smallest) unit of the metric system
  static constexpr TextView baseUnit = "";

  // convert different units to a multiply of the base unit
  static inline uintmax_t convertToBase(TextView const unit);

  // canonicalize the spelling of the metric. e.g. both "b" and "byte" mean the
  // same thing
  // intentionally *not* taking a const unit b/c there might be use cases where
  // the implementation wants to modify the view and then return it
  static inline TextView canonicalize(TextView unit);
};
*/

// namespace enclosing classes that meet the requirements of the Metric concept
namespace metric
{
struct Storage {
  struct UnrecognizedUnit : std::runtime_error {
    UnrecognizedUnit(TextView const msg)
      : std::runtime_error::runtime_error(std::string(msg).c_str()) {}
  };
  static constexpr TextView B  = "B";
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
      throw UnrecognizedUnit(unit); 
    }
  }

  static inline TextView canonicalize(TextView unit) {
    if (!strcasecmp(unit, "b") || !strcasecmp(unit, "byte")) {
      return B;
    } else if (!strcasecmp(unit, "k") || !strcasecmp(unit, "kb")) {
      return KB;
    } else if (!strcasecmp(unit, "m") || !strcasecmp(unit, "mb")) {
      return MB;
    } else if (!strcasecmp(unit, "g") || !strcasecmp(unit, "gb")) {
      return GB;
    } else if (!strcasecmp(unit, "t") || !strcasecmp(unit, "tb")) {
      return TB;
    } else if (!strcasecmp(unit, "p") || !strcasecmp(unit, "pb")) {
      return PB;
    } else {
      throw UnrecognizedUnit(unit); 
      return B;
    }
  }
};

struct Duration {
  struct UnrecognizedUnit : std::runtime_error {
    UnrecognizedUnit(TextView const msg)
      : std::runtime_error::runtime_error(std::string(msg).c_str()) {}
  };
  static constexpr TextView SECOND = "second";
  static constexpr TextView MINUTE = "minute";
  static constexpr TextView HOUR   = "hour";
  static constexpr TextView DAY    = "day";
  static constexpr TextView WEEK   = "week";

  static constexpr TextView baseUnit = "second";

  static uintmax_t convertToBase(TextView const unit) {
    if (unit == SECOND) {
      return static_cast<uintmax_t>(1);
    } else if (unit == MINUTE) {
      return static_cast<uintmax_t>(60);
    } else if (unit == HOUR) {
      return static_cast<uintmax_t>(3'600);
    } else if (unit == DAY) {
      return static_cast<uintmax_t>(86'400);
    } else if (unit == WEEK) {
      return static_cast<uintmax_t>(604'800);
    } else {
      throw UnrecognizedUnit(unit); 
      return 0;
    }
  }
  static inline TextView canonicalize(TextView unit) {
    if (!strcasecmp(unit, "s") || !strcasecmp(unit, "sec") || !strcasecmp(unit, "second")) {
      return SECOND;
    } else if (!strcasecmp(unit, "m") || !strcasecmp(unit, "min") || !strcasecmp(unit, "minute")) {
      return MINUTE;
    } else if (!strcasecmp(unit, "h") || !strcasecmp(unit, "hour")) {
      return HOUR;
    } else if (!strcasecmp(unit, "d") || !strcasecmp(unit, "day")) {
      return DAY;
    } else if (!strcasecmp(unit, "w") || !strcasecmp(unit, "week")) {
      return WEEK;
    } else {
      throw UnrecognizedUnit(unit); 
      return SECOND;
    }
  }
};

} // namespace metric

// (potentially) modifies the pass-in view. returns the unit
template <typename Metric> inline TextView rExtractUnit(TextView &src) noexcept {
  if (isdigit(src.back())) {
    // no unit specification, defaulting to the smallest unit
    return Metric::baseUnit;
  } else {
    // take suffix from right to left -- from the first non-space char
    // (inclusive) to the first digit (exclusive)
    // [0, pos] is a multiplier (string)
    auto const pos  = src.rfind_if([](char const c) { return isdigit(c) || isspace(c); });
    auto const unit = src.substr(pos + 1);
    src             = src.prefix(pos + 1);
    return unit;
  }
}

// (potentially) modifies the pass-in view. returns the multiplier
inline uintmax_t rExtractMultiplier(TextView &src) {
  auto const pos = src.rfind_if([](char const c) { return !isdigit(c); });
  if (pos == std::string_view::npos) {
    // no more non-digit on the left which means this is the left-most
    // multipler. parsing is complete
    auto const multiplier = src;
    src.clear();
    return swoc::svtou(multiplier);
  } else {
    auto const multiplier = src.substr(pos + 1);
    src                   = src.prefix(pos + 1);
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
template <typename Metric> class NumericSuffixParser {
  using self_type = NumericSuffixParser;

public:
  struct ParsingError : std::runtime_error {
    ParsingError(TextView const msg)
      : std::runtime_error::runtime_error(std::string(msg).c_str()) {}
  };

  intmax_t operator()(TextView text) {
    parseSuffixes(text);
    return sumUp();
  }
  void clear() { _suffixes.clear(); }

protected:
  /// Suffix mapping, name -> multiplier.
  std::unordered_map<std::string, uintmax_t> _suffixes;

private:
  uintmax_t sumUp() const {
    uintmax_t sum = 0;
    for (auto const &tuple : _suffixes) {
      sum += Metric::convertToBase(tuple.first) * (tuple.second);
    }
    return sum;
  }

  void parseSuffixes(TextView text) {
    clear();
    while (text.rtrim_if(&isspace)) {
      auto const unit = Metric::canonicalize(rExtractUnit<Metric>(text));
      if (text.rtrim_if(&isspace).empty()) {
        throw ParsingError("Missing multiplier for unit(s)");
      }
      auto const multiplier = rExtractMultiplier(text);
      // no heterogeneous lookup for std::unoredered_map until C++20. had to
      // construct the key type explicitly
      auto const [it, inserted] = _suffixes.try_emplace(std::string(unit), multiplier);
      if (!inserted) {
        it->second += multiplier;
      }
    }
  }
};

TEST_CASE("NumericSuffixParser parsing algorithm", "[libswoc][example][NumericSuffixParser][parsing]") {
  SECTION("1 pair, no default") {
    std::vector<TextView> views = {
      "100M", " 100M", "100M ", " 100M ", "100 M", " 100 M", "100 M ", " 100 M ",
    };
    for (auto view : views) {
      view.rtrim_if(&isspace);
      CHECK("M" == rExtractUnit<metric::Storage>(view));
      view.rtrim_if(&isspace);
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(view));
    }
  }
  SECTION("1 pair, with default") {
    std::vector<TextView> views = {
      "100", " 100", "100 ", " 100 ", "100B", " 100B", "100B ", " 100B ", "100 B", " 100 B", "100 B ", " 100 B ",
    };
    for (auto view : views) {
      view.rtrim_if(&isspace);
      CHECK("B" == rExtractUnit<metric::Storage>(view));
      view.rtrim_if(&isspace);
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(view));
    }
  }
  SECTION("2 pairs, no default") {
    std::vector<TextView> views = {
      "50G100M",    " 50G100M",    "50G100M ",    " 50G100M ",

      "50G 100M",   " 50G 100M",   "50G 100M ",   " 50G 100M ",

      "50 G 100M",  " 50 G 100M",  "50 G 100M ",  " 50 G 100M ",

      "50 G100M",   " 50 G100M",   "50 G100M ",   " 50 G100M ",

      "50G 100 M",  " 50G 100 M",  "50G 100 M ",  " 50G 100 M ",

      "50G100 M",   " 50G100 M",   "50G100 M ",   " 50G100 M ",

      "50 G 100 M", " 50 G 100 M", "50 G 100 M ", " 50 G 100 M ",

      "50 G100 M",  " 50 G100 M",  "50 G100 M ",  " 50 G100 M ",
    };
    for (auto view : views) {
      INFO("src=\"" << view << "\"");
      view.rtrim_if(&isspace);
      CHECK("M" == rExtractUnit<metric::Storage>(view));
      view.rtrim_if(&isspace);
      CHECK(static_cast<uintmax_t>(100) == rExtractMultiplier(view));
      view.rtrim_if(&isspace);
      CHECK("G" == rExtractUnit<metric::Storage>(view));
      view.rtrim_if(&isspace);
      CHECK(static_cast<uintmax_t>(50) == rExtractMultiplier(view));
      view.rtrim_if(&isspace);
      CHECK(view.empty());
    }
  }
  SECTION("2 pairs, with defaults on the right") {
    std::vector<TextView> views = {
      "50G100",   " 50G100",   "50G100 ",   " 50G100 ",

      "50G 100",  " 50G 100",  "50G 100 ",  " 50G 100 ",

      "50 G 100", " 50 G 100", "50 G 100 ", " 50 G 100 ",

      "50 G100",  " 50 G100",  "50 G100 ",  " 50 G100 ",
    };
    for (auto view : views) {
      INFO("src=\"" << view << "\"");
      NumericSuffixParser<metric::Storage> parser;
      CHECK(parser(view) == static_cast<uintmax_t>(50L * (1L << 30) + 100L));
    }
  }
  SECTION("2 pairs, with defaults on the left") {
    std::vector<TextView> views = {
      "50 100M",  " 50 100M",  "50 100M ",  " 50 100M ",

      "50 100 M", " 50 100 M", "50 100 M ", " 50 100 M ",
    };
    for (auto view : views) {
      INFO("src=\"" << view << "\"");
      NumericSuffixParser<metric::Storage> parser;
      CHECK(static_cast<uintmax_t>(50L + 100L * (1L << 20)) == parser(view));
    }
  }
  SECTION("2 pairs, both are defaults") {
    std::vector<TextView> views = {
      "50 100",
      " 50 100",
      "50 100 ",
      " 50 100 ",
    };
    for (auto view : views) {
      INFO("src=\"" << view << "\"");
      NumericSuffixParser<metric::Storage> parser;
      CHECK(static_cast<uintmax_t>(150L) == parser(view));
    }
  }
}

TEST_CASE("NumericSuffixParser end-to-end tests", "[libswoc][example][NumericSuffixParser][e2e tests]") {
  SECTION("Storage metrics") {
    TextView text = "100 kb50G 50 K 20 ";
    NumericSuffixParser<metric::Storage> parser;
    CHECK(parser(text) == static_cast<uintmax_t>(100L * (1 << 10) + 50L * (1 << 30) + 50L * (1 << 10) + 20L));
  }

  SECTION("Time duration metrics") {
    TextView text = " 100 sec10H 5m3s ";
    NumericSuffixParser<metric::Duration> parser;
    CHECK(parser(text) == static_cast<uintmax_t>(100L + 10L * 3'600 + 5 * 60 + 3));
  }
}

TEST_CASE("NumericSuffixParser error handling", "[libswoc][example][NumericSuffixParser][error handlling]") {
  SECTION("Storage metrics") {
    NumericSuffixParser<metric::Storage> parser;

    SECTION("UnrecognizedUnit") {
      TextView text = "hour";
      CHECK_THROWS_AS(parser(text), metric::Storage::UnrecognizedUnit);
    }

    SECTION("Parsing Error") {
      TextView text = "G";
      CHECK_THROWS_AS(parser(text), NumericSuffixParser<metric::Storage>::ParsingError);
    }
  }

  SECTION("Time duration metrics") {
    NumericSuffixParser<metric::Duration> parser;

    SECTION("UnrecognizedUnit") {
      TextView text = "kb";
      CHECK_THROWS_AS(parser(text), metric::Duration::UnrecognizedUnit);
    }

    SECTION("Parsing Error") {
      TextView text = "hour";
      CHECK_THROWS_AS(parser(text), NumericSuffixParser<metric::Duration>::ParsingError);
    }
  }
}
