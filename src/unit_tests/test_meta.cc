/** @file

    Unit tests for ts_meta.h and other meta programming.

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

#include <cstring>

#include "swoc/swoc_meta.h"

#include "swoc/ext/catch.hpp"

template < typename ... > struct is_any_of_1 { static constexpr bool value = false; };
template < typename T, typename T0> struct is_any_of_1<T, T0> { static constexpr bool value = std::is_same<T, T0>::value; };
template < typename T, typename T0, typename ... Rest> struct is_any_of_1<T, T0, Rest...> {
  static constexpr bool value = std::is_same<T, T0>::value ||
                                (sizeof...(Rest) > 0 && is_any_of_1<T, Rest...>::value);
};

// Requires C++17
template < typename T, typename ... Rest >
struct is_any_of_2 {
  static constexpr bool value = std::disjunction<std::is_same<T, Rest>...>::value;
};

struct A {};

struct B {};

struct C {};

struct D {};

TEST_CASE("Meta", "[meta]") {

  REQUIRE(is_any_of_1<A, A, B, C>::value);
  REQUIRE(! is_any_of_1<D, A, B, C>::value);
  REQUIRE(is_any_of_1<A, A>::value);
  REQUIRE(! is_any_of_1<A, D>::value);
  REQUIRE(! is_any_of_1<A>::value);

  REQUIRE(is_any_of_2<A, A, B, C>::value);
  REQUIRE(! is_any_of_2<D, A, B, C>::value);
  REQUIRE(is_any_of_2<A, A>::value);
  REQUIRE(! is_any_of_2<A, D>::value);
  REQUIRE(! is_any_of_2<A>::value);
}
