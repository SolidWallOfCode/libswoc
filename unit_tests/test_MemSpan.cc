// SPDX-License-Identifier: Apache-2.0
/** @file

    MemSpan unit tests.

*/

#include <iostream>
#include "swoc/MemSpan.h"
#include "catch.hpp"

using swoc::MemSpan;

TEST_CASE("MemSpan", "[libswoc][MemSpan]")
{
  int32_t idx[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  char buff[1024];

  MemSpan<char> span(buff, sizeof(buff));
  MemSpan<char> left = span.prefix(512);
  REQUIRE(left.size() == 512);
  REQUIRE(span.size() == 1024);
  span.remove_prefix(512);
  REQUIRE(span.size() == 512);
  REQUIRE(left.end() == span.begin());

  left.assign(buff, sizeof(buff));
  span = left.suffix(768);
  REQUIRE(span.size() == 768);
  left.remove_suffix(768);
  REQUIRE(left.end() == span.begin());
  REQUIRE(left.size() + span.size() == 1024);

  MemSpan<int32_t> idx_span(idx);
  REQUIRE(idx_span.count() == 11);
  REQUIRE(idx_span.size() == sizeof(idx));
  REQUIRE(idx_span.data() == idx);

  auto sp2 = idx_span.rebind<int16_t>();
  REQUIRE(sp2.size() == idx_span.size());
  REQUIRE(sp2.count() == 2 * idx_span.count());
  REQUIRE(sp2[0] == 0);
  REQUIRE(sp2[1] == 0);
  // exactly one of { le, be } must be true.
  bool le = sp2[2] == 1 && sp2[3] == 0;
  bool be = sp2[2] == 0 && sp2[3] == 1;
  REQUIRE(le != be);
  auto idx2 = sp2.rebind<int32_t>(); // still the same if converted back to original?
  REQUIRE(idx_span.is_same(idx2));

  // Verify attempts to rebind on non-integral sized arrays fails.
  span.assign(buff, 1022);
  REQUIRE(span.size() == 1022);
  REQUIRE(span.count() == 1022);
  auto vs = span.rebind<void>();
  REQUIRE_THROWS_AS(span.rebind<uint32_t>(), std::invalid_argument);
  REQUIRE_THROWS_AS(vs.rebind<uint32_t>(), std::invalid_argument);
  vs.rebind<void>(); // check for void -> void rebinding.

  // Check for defaulting to a void rebind.
  vs = span.rebind();
  REQUIRE(vs.size() == 1022);

  // Check for assignment to void.
  vs = span;
  REQUIRE(vs.size() == 1022);

  // Test array constructors.
  MemSpan<char> a{buff};
  REQUIRE(a.size() == sizeof(buff));
  REQUIRE(a.data() == buff);
  float floats[] = {1.1, 2.2, 3.3, 4.4, 5.5};
  MemSpan<float> fspan{floats};
  REQUIRE(fspan.count() == 5);
  REQUIRE(fspan[3] == 4.4f);
  MemSpan<float> f2span{floats, floats + 5};
  REQUIRE(fspan.data() == f2span.data());
  REQUIRE(fspan.count() == f2span.count());
  REQUIRE(fspan.is_same(f2span));
};

TEST_CASE("MemSpan<void>", "[libswoc][MemSpan]")
{
  char buff[1024];

  MemSpan<void> span(buff, sizeof(buff));
  auto left = span.prefix(512);
  REQUIRE(left.size() == 512);
  REQUIRE(span.size() == 1024);
  span.remove_prefix(512);
  REQUIRE(span.size() == 512);
  REQUIRE(left.data_end() == span.data());

  left.assign(buff, sizeof(buff));
  span = left.suffix(700);
  REQUIRE(span.size() == 700);
  left.remove_suffix(700);
  REQUIRE(left.data_end() == span.data());
  REQUIRE(left.size() + span.size() == 1024);

};

TEST_CASE("MemSpan conversions", "[libswoc][MemSpan]")
{
  std::array<int, 10> a1;
  auto const & ra1 = a1;
  auto ms1 = MemSpan<int>(a1); // construct from array
  [[maybe_unused]] auto ms2 = MemSpan(a1); // construct from array, deduction guide
  [[maybe_unused]] auto ms3 = MemSpan<int const>(ra1); // construct from const array
  [[maybe_unused]] auto ms4 = MemSpan(ra1); // construct from const array, deduction guided.
  // Construct a span of constant from a const ref to an array with non-const type.
  MemSpan<const int> ms5 { ra1 };
  // Construct a span of constant from a ref to an array with non-const type.
  MemSpan<const int> ms6 { a1 };

  [[maybe_unused]] MemSpan<int const> c1 = ms1; // Conversion from T to T const.
}
