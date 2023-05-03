// SPDX-License-Identifier: Apache-2.0
// Copyright Verizon Media 2020
/** @file

    Lexicon unit tests.
*/

#include <iostream>

#include "swoc/Lexicon.h"
#include "catch.hpp"

// Example code for documentatoin
// ---

enum class Example { INVALID, Value_0, Value_1, Value_2, Value_3 };

using ExampleNames = swoc::Lexicon<Example>;

namespace
{
[[maybe_unused]] ExampleNames Static_Names {
  {Example::Value_0, {"zero", "0"}}, {Example::Value_1, {"one", "1"}}, {Example::Value_2, {"two", "2"}},
    {Example::Value_3, {"three", "3"}},
  {
    Example::INVALID, { "INVALID" }
  }
};
}

TEST_CASE("Lexicon", "[libts][Lexicon]")
{
  ExampleNames exnames{{Example::Value_0, {"zero", "0"}},
                       {Example::Value_1, {"one", "1"}},
                       {Example::Value_2, {"two", "2"}},
                       {Example::Value_3, {"three", "3"}},
                       {Example::INVALID, {"INVALID"}}};

  ExampleNames exnames2{{Example::Value_0, "zero"},
                        {Example::Value_1, "one"},
                        {Example::Value_2, "two"},
                        {Example::Value_3, "three"},
                        {Example::INVALID, "INVALID"}};

  // Check constructing with just defaults.
  ExampleNames def_names_1 { Example::INVALID };
  ExampleNames def_names_2 { "INVALID" };
  ExampleNames def_names_3 { Example::INVALID, "INVALID" };

  exnames.set_default(Example::INVALID).set_default("INVALID");

  REQUIRE(exnames[Example::INVALID] == "INVALID");
  REQUIRE(exnames[Example::Value_0] == "zero");
  REQUIRE(exnames["zero"] == Example::Value_0);
  REQUIRE(exnames["Zero"] == Example::Value_0);
  REQUIRE(exnames["ZERO"] == Example::Value_0);
  REQUIRE(exnames["one"] == Example::Value_1);
  REQUIRE(exnames["1"] == Example::Value_1);
  REQUIRE(exnames["Evil Dave"] == Example::INVALID);
  REQUIRE(exnames[static_cast<Example>(0xBADD00D)] == "INVALID");
  REQUIRE(exnames[exnames[static_cast<Example>(0xBADD00D)]] == Example::INVALID);

  REQUIRE(def_names_1["zero"] == Example::INVALID);
  REQUIRE(def_names_2[Example::Value_0] == "INVALID");
  REQUIRE(def_names_3["zero"] == Example::INVALID);
  REQUIRE(def_names_3[Example::Value_0] == "INVALID");

  enum class Radio { INVALID, ALPHA, BRAVO, CHARLIE, DELTA };
  using Lex = swoc::Lexicon<Radio>;
  Lex lex({{Radio::INVALID, {"Invalid"}},
           {Radio::ALPHA, {"Alpha"}},
           {Radio::BRAVO, {"Bravo", "Beta"}},
           {Radio::CHARLIE, {"Charlie"}},
           {Radio::DELTA, {"Delta"}}});

  // test structured binding for iteration.
  for ([[maybe_unused]] auto const &[key, name] : lex) {
  }
};

// ---
// End example code.

enum Values { NoValue, LowValue, HighValue, Priceless };
enum Hex { A, B, C, D, E, F, INVALID };

using ValueLexicon = swoc::Lexicon<Values>;
using HexLexicon   = swoc::Lexicon<Hex>;

TEST_CASE("Lexicon Constructor", "[libts][Lexicon]")
{
  // Construct with a secondary name for NoValue
  ValueLexicon vl{{NoValue, {"NoValue", "garbage"}}, {LowValue, {"LowValue"}}};

  REQUIRE("LowValue" == vl[LowValue]);                 // Primary name
  REQUIRE(NoValue == vl["NoValue"]);                   // Primary name
  REQUIRE(NoValue == vl["garbage"]);                   // Secondary name
  REQUIRE_THROWS_AS(vl["monkeys"], std::domain_error); // No default, so throw.
  vl.set_default(NoValue);                             // Put in a default.
  REQUIRE(NoValue == vl["monkeys"]);                   // Returns default instead of throw
  REQUIRE(LowValue == vl["lowVALUE"]);                 // Check case insensitivity.

  REQUIRE(NoValue == vl["HighValue"]);               // Not defined yet.
  vl.define(HighValue, {"HighValue", "High_Value"}); // Add it.
  REQUIRE(HighValue == vl["HighValue"]);             // Verify it's there and is case insensitive.
  REQUIRE(HighValue == vl["highVALUE"]);
  REQUIRE(HighValue == vl["HIGH_VALUE"]);
  REQUIRE("HighValue" == vl[HighValue]); // Verify value -> primary name.

  // A few more checks on primary/secondary.
  REQUIRE(NoValue == vl["Priceless"]);
  REQUIRE(NoValue == vl["unique"]);
  vl.define(Priceless, "Priceless", "Unique");
  REQUIRE("Priceless" == vl[Priceless]);
  REQUIRE(Priceless == vl["unique"]);

  // Check default handlers.
  using LL         = swoc::Lexicon<Hex>;
  bool bad_value_p = false;
  LL ll_1({{A, "A"}, {B, "B"}, {C, "C"}, {E, "E"}});
  ll_1.set_default([&bad_value_p](std::string_view name) mutable -> Hex {
    bad_value_p = true;
    return INVALID;
  });
  ll_1.set_default([&bad_value_p](Hex value) mutable -> std::string_view {
    bad_value_p = true;
    return "INVALID";
  });
  REQUIRE(bad_value_p == false);
  REQUIRE(INVALID == ll_1["F"]);
  REQUIRE(bad_value_p == true);
  bad_value_p = false;
  REQUIRE("INVALID" == ll_1[F]);
  REQUIRE(bad_value_p == true);
  bad_value_p = false;
  // Verify that INVALID / "INVALID" are equal because of the default handlers.
  REQUIRE("INVALID" == ll_1[INVALID]);
  REQUIRE(INVALID == ll_1["INVALID"]);
  REQUIRE(bad_value_p == true);
  // Define the value/name, verify the handlers are *not* invoked.
  ll_1.define(INVALID, "INVALID");
  bad_value_p = false;
  REQUIRE("INVALID" == ll_1[INVALID]);
  REQUIRE(INVALID == ll_1["INVALID"]);
  REQUIRE(bad_value_p == false);

  ll_1.define({D, "D"});          // Pair style
  ll_1.define({F, {"F", "0xf"}}); // Definition style
  REQUIRE(ll_1[D] == "D");
  REQUIRE(ll_1["0XF"] == F);

  // iteration
  std::bitset<INVALID + 1> mark;
  for (auto [value, name] : ll_1) {
    if (mark[value]) {
      std::cerr << "Lexicon: " << name << ':' << value << " double iterated" << std::endl;
      mark.reset();
      break;
    }
    mark[value] = true;
  }
  REQUIRE(mark.all());

  ValueLexicon v2(std::move(vl));
  REQUIRE(vl.count() == 0);

  REQUIRE("LowValue" == v2[LowValue]);                 // Primary name
  REQUIRE(NoValue == v2["NoValue"]);                   // Primary name
  REQUIRE(NoValue == v2["garbage"]);                   // Secondary name

  REQUIRE(HighValue == v2["highVALUE"]);
  REQUIRE(HighValue == v2["HIGH_VALUE"]);
  REQUIRE("HighValue" == v2[HighValue]); // Verify value -> primary name.

  // A few more checks on primary/secondary.
  REQUIRE("Priceless" == v2[Priceless]);
  REQUIRE(Priceless == v2["unique"]);

};

TEST_CASE("Lexicon Constructor 2", "[libts][Lexicon]")
{
  // Check the various construction cases
  // No defaults, value default, name default, both, both the other way.
  const HexLexicon v1(HexLexicon::with_multi{{A, {"A", "ten"}}, {B, {"B", "eleven"}}});

  const HexLexicon v2(HexLexicon::with_multi{{A, {"A", "ten"}}, {B, {"B", "eleven"}}}, INVALID);

  const HexLexicon v3(HexLexicon::with_multi{{A, {"A", "ten"}}, {B, {"B", "eleven"}}}, "Invalid");

  const HexLexicon v4(HexLexicon::with_multi{{A, {"A", "ten"}}, {B, {"B", "eleven"}}}, "Invalid", INVALID);

  const HexLexicon v5{HexLexicon::with_multi{{A, {"A", "ten"}}, {B, {"B", "eleven"}}}, INVALID, "Invalid"};

  REQUIRE(v1["a"] == A);
  REQUIRE(v2["q"] == INVALID);
  REQUIRE(v3[C] == "Invalid");
  REQUIRE(v4["q"] == INVALID);
  REQUIRE(v4[C] == "Invalid");
  REQUIRE(v5["q"] == INVALID);
  REQUIRE(v5[C] == "Invalid");
}
