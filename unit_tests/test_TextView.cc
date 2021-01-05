/** @file

    TextView unit tests.

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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "swoc/TextView.h"
#include "catch.hpp"

using swoc::TextView;
using namespace std::literals;
using namespace swoc::literals;

TEST_CASE("TextView Constructor", "[libswoc][TextView]")
{
  static std::string base = "Evil Dave Rulez!";
  unsigned ux             = base.size();
  TextView tv(base);
  TextView a{"Evil Dave Rulez"};
  TextView b{base.data(), base.size()};
  TextView c{std::string_view(base)};
  constexpr TextView d{"Grigor!"sv};
  TextView e{base.data(), 15};
  TextView f(base.data(), 15);
  TextView u{base.data(), ux};
};

TEST_CASE("TextView Operations", "[libswoc][TextView]")
{
  TextView tv{"Evil Dave Rulez"};
  TextView tv_lower{"evil dave rulez"};
  TextView nothing;
  size_t off;

  REQUIRE(tv.find('l') == 3);
  off = tv.find_if([](char c) { return c == 'D'; });
  REQUIRE(off == tv.find('D'));

  REQUIRE(tv);
  REQUIRE(!tv == false);
  if (nothing) {
    REQUIRE(nullptr == "bad operator bool on TextView");
  }
  REQUIRE(!nothing == true);
  REQUIRE(nothing.empty() == true);

  REQUIRE(memcmp(tv, tv) == 0);
  REQUIRE(memcmp(tv, tv_lower) != 0);
  REQUIRE(strcmp(tv, tv) == 0);
  REQUIRE(strcmp(tv, tv_lower) != 0);
  REQUIRE(strcasecmp(tv, tv) == 0);
  REQUIRE(strcasecmp(tv, tv_lower) == 0);
  REQUIRE(strcasecmp(nothing, tv) != 0);
}

TEST_CASE("TextView Trimming", "[libswoc][TextView]")
{
  TextView tv("  Evil Dave Rulz   ...");
  TextView tv2{"More Text1234567890"};
  REQUIRE("Evil Dave Rulz   ..." == TextView(tv).ltrim_if(&isspace));
  REQUIRE(tv2 == TextView{tv2}.ltrim_if(&isspace));
  REQUIRE("More Text" == TextView{tv2}.rtrim_if(&isdigit));
  REQUIRE("  Evil Dave Rulz   " == TextView(tv).rtrim('.'));
  REQUIRE("Evil Dave Rulz" == TextView(tv).trim(" ."));

  tv.assign("\r\n");
  tv.rtrim_if([](char c) -> bool { return c == '\r' || c == '\n'; });
  REQUIRE(tv.size() == 0);

  tv.assign("...");
  tv.rtrim('.');
  REQUIRE(tv.size() == 0);

  tv.assign(".,,.;.");
  tv.rtrim(";,."_tv);
  REQUIRE(tv.size() == 0);
}

TEST_CASE("TextView Find", "[libswoc][TextView]")
{
  TextView addr{"172.29.145.87:5050"};
  REQUIRE(addr.find(':') == 13);
  REQUIRE(addr.rfind(':') == 13);
  REQUIRE(addr.find('.') == 3);
  REQUIRE(addr.rfind('.') == 10);
}

TEST_CASE("TextView Affixes", "[libswoc][TextView]")
{
  TextView s; // scratch.
  TextView tv1("0123456789;01234567890");
  TextView prefix{tv1.prefix(10)};

  REQUIRE("0123456789" == prefix);
  REQUIRE("67890" == tv1.suffix(5));
  REQUIRE(tv1 == tv1.prefix(9999));
  REQUIRE(tv1 == tv1.suffix(9999));

  TextView tv2 = tv1.prefix_at(';');
  REQUIRE(tv2 == "0123456789");
  REQUIRE(tv1.prefix_at('z').empty());
  REQUIRE(tv1.suffix_at('z').empty());

  s = tv1;
  REQUIRE(s.remove_prefix(10) == ";01234567890");
  s = tv1;
  REQUIRE(s.remove_prefix(9999).empty());
  s = tv1;
  REQUIRE(s.remove_suffix(11) == "0123456789;");
  s = tv1;
  s.remove_suffix(9999);
  REQUIRE(s.empty());
  REQUIRE(s.data() == tv1.data());

  TextView right{tv1};
  TextView left{right.split_prefix_at(';')};
  REQUIRE(right.size() == 11);
  REQUIRE(left.size() == 10);

  TextView tv3 = "abcdefg:gfedcba";
  left         = tv3;
  right        = left.split_suffix_at(";:,");
  TextView pre{tv3}, post{pre.split_suffix(7)};
  REQUIRE(right.size() == 7);
  REQUIRE(left.size() == 7);
  REQUIRE(left == "abcdefg");
  REQUIRE(right == "gfedcba");

  TextView addr1{"[fe80::fc54:ff:fe60:d886]"};
  TextView addr2{"[fe80::fc54:ff:fe60:d886]:956"};
  TextView addr3{"192.168.1.1:5050"};

  TextView t = addr1;
  ++t;
  REQUIRE("fe80::fc54:ff:fe60:d886]" == t);
  TextView a = t.take_prefix_at(']');
  REQUIRE("fe80::fc54:ff:fe60:d886" == a);
  REQUIRE(t.empty());

  t = addr2;
  ++t;
  a = t.take_prefix_at(']');
  REQUIRE("fe80::fc54:ff:fe60:d886" == a);
  REQUIRE(':' == *t);
  ++t;
  REQUIRE("956" == t);

  t = addr3;
  TextView sf{t.suffix_at(':')};
  REQUIRE("5050" == sf);
  REQUIRE(t == addr3);

  t = addr3;
  s = t.split_suffix(4);
  REQUIRE("5050" == s);
  REQUIRE("192.168.1.1" == t);

  t = addr3;
  s = t.split_suffix_at(':');
  REQUIRE("5050" == s);
  REQUIRE("192.168.1.1" == t);

  t = addr3;
  s = t.split_suffix_at('Q');
  REQUIRE(s.empty());
  REQUIRE(t == addr3);

  t = addr3;
  s = t.take_suffix_at(':');
  REQUIRE("5050" == s);
  REQUIRE("192.168.1.1" == t);

  t = addr3;
  s = t.take_suffix_at('Q');
  REQUIRE(s == addr3);
  REQUIRE(t.empty());

  auto is_sep{[](char c) { return isspace(c) || ',' == c || ';' == c; }};
  TextView token;
  t = ";; , ;;one;two,th:ree  four,, ; ,,f-ive="sv;
  // Do an unrolled loop.
  REQUIRE(!t.ltrim_if(is_sep).empty());
  REQUIRE(t.take_prefix_if(is_sep) == "one");
  REQUIRE(!t.ltrim_if(is_sep).empty());
  REQUIRE(t.take_prefix_if(is_sep) == "two");
  REQUIRE(!t.ltrim_if(is_sep).empty());
  REQUIRE(t.take_prefix_if(is_sep) == "th:ree");
  REQUIRE(!t.ltrim_if(is_sep).empty());
  REQUIRE(t.take_prefix_if(is_sep) == "four");
  REQUIRE(!t.ltrim_if(is_sep).empty());
  REQUIRE(t.take_prefix_if(is_sep) == "f-ive=");
  REQUIRE(t.empty());

  // Simulate pulling off FQDN pieces in reverse order from a string_view.
  // Simulates operations in HostLookup.cc, where the use of string_view
  // necessitates this workaround of failures in the string_view API.
  std::string_view fqdn{"bob.ne1.corp.ngeo.com"};
  TextView elt{TextView{fqdn}.take_suffix_at('.')};
  REQUIRE(elt == "com");
  fqdn.remove_suffix(std::min(fqdn.size(), elt.size() + 1));

  // Unroll loop for testing.
  elt = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(elt == "ngeo");
  fqdn.remove_suffix(std::min(fqdn.size(), elt.size() + 1));
  elt = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(elt == "corp");
  fqdn.remove_suffix(std::min(fqdn.size(), elt.size() + 1));
  elt = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(elt == "ne1");
  fqdn.remove_suffix(std::min(fqdn.size(), elt.size() + 1));
  elt = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(elt == "bob");
  fqdn.remove_suffix(std::min(fqdn.size(), elt.size() + 1));
  elt = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(elt.empty());

  // Check some edge cases.
  fqdn  = "."sv;
  token = TextView{fqdn}.take_suffix_at('.');
  REQUIRE(token.size() == 0);
  REQUIRE(token.empty());

  s = "."sv;
  REQUIRE(s.size() == 1);
  REQUIRE(s.rtrim('.').empty());
  token = s.take_suffix_at('.');
  REQUIRE(token.size() == 0);
  REQUIRE(token.empty());

  s = "."sv;
  REQUIRE(s.size() == 1);
  REQUIRE(s.ltrim('.').empty());
  token = s.take_prefix_at('.');
  REQUIRE(token.size() == 0);
  REQUIRE(token.empty());

  s = ".."sv;
  REQUIRE(s.size() == 2);
  token = s.take_suffix_at('.');
  REQUIRE(token.size() == 0);
  REQUIRE(token.empty());
  REQUIRE(s.size() == 1);

  auto is_not_alnum = [](char c) { return !isalnum(c); };

  s = "file.cc";
  REQUIRE(s.suffix_at('.') == "cc");
  REQUIRE(s.suffix_if(is_not_alnum) == "cc");
  REQUIRE(s.prefix_at('.') == "file");
  REQUIRE(s.prefix_if(is_not_alnum) == "file");
  s.remove_suffix_at('.');
  REQUIRE(s == "file");
  s = "file.cc.org.123";
  REQUIRE(s.suffix_at('.') == "123");
  REQUIRE(s.prefix_at('.') == "file");
  s.remove_suffix_if(is_not_alnum);
  REQUIRE(s == "file.cc.org");
  s.remove_suffix_at('.');
  REQUIRE(s == "file.cc");
  s.remove_prefix_at('.');
  REQUIRE(s == "cc");
  s = "file.cc.org.123";
  s.remove_prefix_if(is_not_alnum);
  REQUIRE(s == "cc.org.123");
  s.remove_suffix_at('!');
  REQUIRE(s == "cc.org.123");
  s = "file.cc.org";
  s.remove_prefix_at('!');
  REQUIRE(s == "file.cc.org");

  static constexpr TextView ctv {"http://delain.nl/albums/Lucidity.html"};
  static constexpr TextView ctv_scheme{ctv.prefix(4)};
  static constexpr TextView ctv_stem{ctv.suffix(4)};
  static constexpr TextView ctv_host{ctv.substr(7, 9)};
  REQUIRE(ctv.starts_with("http"_tv) == true);
  REQUIRE(ctv.ends_with(".html") == true);
  REQUIRE(ctv.starts_with("https"_tv) == false);
  REQUIRE(ctv.ends_with(".jpg") == false);
  REQUIRE(ctv.starts_with_nocase("HttP"_tv) == true);
  REQUIRE(ctv.starts_with_nocase("HttP") == true);
  REQUIRE(ctv.starts_with("HttP") == false);
  REQUIRE(ctv.starts_with("http") == true);
  REQUIRE(ctv.starts_with('h') == true);
  REQUIRE(ctv.starts_with('H') == false);
  REQUIRE(ctv.starts_with_nocase('H') == true);
  REQUIRE(ctv.starts_with('q') == false);
  REQUIRE(ctv.starts_with_nocase('Q') == false);
  REQUIRE(ctv.ends_with("htML"_tv) == false);
  REQUIRE(ctv.ends_with_nocase("htML"_tv) == true);
  REQUIRE(ctv.ends_with("htML") == false);
  REQUIRE(ctv.ends_with_nocase("htML") == true);

  REQUIRE(ctv_scheme == "http"_tv);
  REQUIRE(ctv_stem == "html"_tv);
  REQUIRE(ctv_host == "delain.nl"_tv);

  // Checking that constexpr works for this constructor as long as npos isn't used.
  static constexpr TextView ctv2 {"http://delain.nl/albums/Interlude.html", 38};
  TextView ctv4 {"http://delain.nl/albums/Interlude.html", 38};
  // This doesn't compile because it causes strlen to be called which isn't constexpr compatible.
  //static constexpr TextView ctv3 {"http://delain.nl/albums/Interlude.html", TextView::npos};
  // This works because it's not constexpr.
  TextView ctv3 {"http://delain.nl/albums/Interlude.html", TextView::npos};
  REQUIRE(ctv2 == ctv3);
};

TEST_CASE("TextView Formatting", "[libswoc][TextView]")
{
  TextView a("01234567");
  {
    std::ostringstream buff;
    buff << '|' << a << '|';
    REQUIRE(buff.str() == "|01234567|");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(5) << a << '|';
    REQUIRE(buff.str() == "|01234567|");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(12) << a << '|';
    REQUIRE(buff.str() == "|    01234567|");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(12) << std::right << a << '|';
    REQUIRE(buff.str() == "|    01234567|");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(12) << std::left << a << '|';
    REQUIRE(buff.str() == "|01234567    |");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(12) << std::right << std::setfill('_') << a << '|';
    REQUIRE(buff.str() == "|____01234567|");
  }
  {
    std::ostringstream buff;
    buff << '|' << std::setw(12) << std::left << std::setfill('_') << a << '|';
    REQUIRE(buff.str() == "|01234567____|");
  }
}

TEST_CASE("TextView Conversions", "[libswoc][TextView]")
{
  TextView n  = "   956783";
  TextView n2 = n;
  TextView n3 = "031";
  TextView n4 = "13f8q";
  TextView n5 = "0x13f8";
  TextView n6 = "0X13f8";
  TextView n7 = "-2345679";
  TextView n8 = "+2345679";
  TextView x;
  n2.ltrim_if(&isspace);

  REQUIRE(956783 == svtoi(n));
  REQUIRE(956783 == svtoi(n2));
  REQUIRE(956783 == svtoi(n2, &x));
  REQUIRE(x.data() == n2.data());
  REQUIRE(x.size() == n2.size());
  REQUIRE(0x13f8 == svtoi(n4, &x, 16));
  REQUIRE(x == "13f8");
  REQUIRE(0x13f8 == svtoi(n5));
  REQUIRE(0x13f8 == svtoi(n6));

  REQUIRE(25 == svtoi(n3));
  REQUIRE(31 == svtoi(n3, nullptr, 10));

  REQUIRE(-2345679 == svtoi(n7));
  REQUIRE(-2345679 == svtoi(n7, &x));
  REQUIRE(x == n7);
  REQUIRE(2345679 == svtoi(n8));
  REQUIRE(2345679 == svtoi(n8, &x));
  REQUIRE(x == n8);
  REQUIRE(0b10111 == svtoi("0b10111"_tv));

  x = n4;
  REQUIRE(13 == swoc::svto_radix<10>(x));
  REQUIRE(x.size() + 2 == n4.size());
  x = n4;
  REQUIRE(0x13f8 == swoc::svto_radix<16>(x));
  REQUIRE(x.size() + 4 == n4.size());
  x = n4;
  REQUIRE(7 == swoc::svto_radix<4>(x));
  REQUIRE(x.size() + 2 == n4.size());
  x = n3;
  REQUIRE(31 == swoc::svto_radix<10>(x));
  REQUIRE(x.size() == 0);
  x = n3;
  REQUIRE(25 == swoc::svto_radix<8>(x));
  REQUIRE(x.size() == 0);

  // floating point is never exact, so "good enough" is all that is measureable. This checks the
  // value is within one epsilon (minimum change possible) of the compiler generated value.
  auto fcmp = [](double lhs, double rhs) {
      double tolerance = std::max( { 1.0, std::fabs(lhs) , std::fabs(rhs) } ) * std::numeric_limits<double>::epsilon();
      return std::fabs(lhs - rhs) <= tolerance ;
  };

  REQUIRE(1.0 == swoc::svtod("1.0"));
  REQUIRE(2.0 == swoc::svtod("2.0"));
  REQUIRE(true == fcmp(0.1, swoc::svtod("0.1")));
  REQUIRE(true == fcmp(0.1, swoc::svtod(".1")));
  REQUIRE(true == fcmp(0.02, swoc::svtod("0.02")));
  REQUIRE(true == fcmp(2.718281828, swoc::svtod("2.718281828")));
  REQUIRE(true == fcmp(-2.718281828, swoc::svtod("-2.718281828")));
  REQUIRE(true == fcmp(2.718281828, swoc::svtod("+2.718281828")));
  REQUIRE(true == fcmp(0.004, swoc::svtod("4e-3")));
  REQUIRE(true == fcmp(4e-3, swoc::svtod("4e-3")));
  REQUIRE(true == fcmp(500000, swoc::svtod("5e5")));
  REQUIRE(true == fcmp(5e5, swoc::svtod("5e+5")));
  REQUIRE(true == fcmp(678900, swoc::svtod("6.789E5")));
  REQUIRE(true == fcmp(6.789e5, swoc::svtod("6.789E+5")));
}

TEST_CASE("TransformView", "[libswoc][TransformView]")
{
  std::string_view source{"Evil Dave Rulz"};
  swoc::TransformView<int (*)(int), std::string_view> xv1(&tolower, source);
  auto xv2 = swoc::transform_view_of(&tolower, source);
  TextView tv{source};

  REQUIRE(xv1 == xv2);

  bool match_p = true;
  while (xv1) {
    if (*xv1 != tolower(*tv)) {
      match_p = false;
      break;
    }
    ++xv1;
    ++tv;
  }
  REQUIRE(match_p);
  REQUIRE(xv1 != xv2);

  tv      = source;
  match_p = true;
  while (xv2) {
    if (*xv2 != tolower(*tv)) {
      match_p = false;
      break;
    }
    ++xv2;
    ++tv;
  }
  REQUIRE(match_p);
};
