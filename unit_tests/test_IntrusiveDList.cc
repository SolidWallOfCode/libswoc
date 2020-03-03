/** @file

    IntrusiveDList unit tests.

    @section license License

    Licensed to the Apache Software Foundation (ASF) under one or more contributor license
    agreements.  See the NOTICE file distributed with this work for additional information regarding
    copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0
    (the "License"); you may not use this file except in compliance with the License.  You may
    obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software distributed under the
    License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
    express or implied. See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <iostream>
#include <string_view>
#include <string>
#include <algorithm>

#include "swoc/IntrusiveDList.h"
#include "swoc/bwf_base.h"

#include "catch.hpp"

using swoc::IntrusiveDList;

namespace
{
struct Thing {
  std::string _payload;
  Thing *_next{nullptr};
  Thing *_prev{nullptr};

  Thing(std::string_view text) : _payload(text) {}

  struct Linkage {
    static Thing *&
    next_ptr(Thing *t)
    {
      return t->_next;
    }

    static Thing *&
    prev_ptr(Thing *t)
    {
      return t->_prev;
    }
  };
};

using ThingList = IntrusiveDList<Thing::Linkage>;

} // namespace

TEST_CASE("IntrusiveDList", "[libswoc][IntrusiveDList]")
{
  ThingList list;
  int n;

  REQUIRE(list.count() == 0);
  REQUIRE(list.head() == nullptr);
  REQUIRE(list.tail() == nullptr);
  REQUIRE(list.begin() == list.end());
  REQUIRE(list.empty());

  n = 0;
  for ([[maybe_unused]] auto &thing : list)
    ++n;
  REQUIRE(n == 0);
  // Check const iteration (mostly compile checks here).
  for ([[maybe_unused]] auto &thing : static_cast<ThingList const &>(list))
    ++n;
  REQUIRE(n == 0);

  list.append(new Thing("one"));
  REQUIRE(list.begin() != list.end());
  REQUIRE(list.tail() == list.head());

  list.prepend(new Thing("two"));
  REQUIRE(list.count() == 2);
  REQUIRE(list.head()->_payload == "two");
  REQUIRE(list.tail()->_payload == "one");
  list.prepend(list.take_tail());
  REQUIRE(list.head()->_payload == "one");
  REQUIRE(list.tail()->_payload == "two");
  list.insert_after(list.head(), new Thing("middle"));
  list.insert_before(list.tail(), new Thing("muddle"));
  REQUIRE(list.count() == 4);
  auto spot = list.begin();
  REQUIRE((*spot++)._payload == "one");
  REQUIRE((*spot++)._payload == "middle");
  REQUIRE((*spot++)._payload == "muddle");
  REQUIRE((*spot++)._payload == "two");
  REQUIRE(spot == list.end());
  spot = list.begin(); // verify assignment works.

  Thing *thing = list.take_head();
  REQUIRE(thing->_payload == "one");
  REQUIRE(list.count() == 3);
  REQUIRE(list.head() != nullptr);
  REQUIRE(list.head()->_payload == "middle");

  list.prepend(thing);
  list.erase(list.head());
  REQUIRE(list.count() == 3);
  REQUIRE(list.head() != nullptr);
  REQUIRE(list.head()->_payload == "middle");
  list.prepend(thing);

  thing = list.take_tail();
  REQUIRE(thing->_payload == "two");
  REQUIRE(list.count() == 3);
  REQUIRE(list.tail() != nullptr);
  REQUIRE(list.tail()->_payload == "muddle");

  list.append(thing);
  list.erase(list.tail());
  REQUIRE(list.count() == 3);
  REQUIRE(list.tail() != nullptr);
  REQUIRE(list.tail()->_payload == "muddle");
  REQUIRE(list.head()->_payload == "one");

  list.insert_before(list.end(), new Thing("trailer"));
  REQUIRE(list.count() == 4);
  REQUIRE(list.tail()->_payload == "trailer");

  for ( auto const& elt : list) {

  }

}
