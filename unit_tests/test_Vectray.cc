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
}
