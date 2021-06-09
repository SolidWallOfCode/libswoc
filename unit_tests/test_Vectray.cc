// SPDX-License-Identifier: Apache-2.0
/** @file

    MemSpan unit tests.

*/

#include <iostream>
#include "swoc/Vectray.h"
#include "catch.hpp"

using swoc::Vectray;

TEST_CASE("Vectray", "[libswoc][Vectray]")
{
  struct Thing {
    unsigned n = 56;
  };

  Vectray<Thing, 1> unit_thing;

  REQUIRE(unit_thing.size() == 0);

  unit_thing.push_back(Thing{0});
  REQUIRE(unit_thing.size() == 1);
  unit_thing.push_back(Thing{1});
  REQUIRE(unit_thing.size() == 2);

  // Check via indexed access.
  for ( unsigned idx = 0 ; idx < unit_thing.size() ; ++idx ) {
    REQUIRE(unit_thing[idx].n == idx);
  }

  // Check via container access.
  unsigned n = 0;
  for ( auto const& thing : unit_thing ) {
    REQUIRE(thing.n == n);
    ++n;
  }
  REQUIRE(n == unit_thing.size());
}
