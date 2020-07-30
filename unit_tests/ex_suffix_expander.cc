// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Numeric suffix expander parsing example.

    This parses a string consisting of integers with suffixes. The suffixes denote a multiplier. The result of the parsing is
    an integer value that is the sum of each integer and the token multiplier.
*/

#include "swoc/TextView.h"
#include "swoc/Lexicon.h"
#include "swoc/swoc_file.h"
#include "catch.hpp"

using swoc::TextView;
using swoc::Lexicon;

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
  Lexicon<std::string> _tokens;
};
