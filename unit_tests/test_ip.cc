// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file

    IP address support testing.
*/

#include "catch.hpp"

#include <set>

#include <swoc/TextView.h>
#include <swoc/swoc_ip.h>
#include <swoc/bwf_ip.h>
#include <swoc/bwf_std.h>
#include <swoc/swoc_file.h>

using namespace std::literals;
using namespace swoc::literals;
using swoc::TextView;
using swoc::IPEndpoint;

using swoc::IP4Addr;
using swoc::IP4Range;

using swoc::IP6Addr;
using swoc::IP6Range;

using swoc::IPAddr;
using swoc::IPRange;

using swoc::IPMask;

using W = swoc::LocalBufferWriter<256>;

TEST_CASE("Basic IP", "[libswoc][ip]") {
  // Use TextView because string_view(nullptr) fails. Gah.
  struct ip_parse_spec {
    TextView hostspec;
    TextView host;
    TextView port;
    TextView rest;
  };

  constexpr ip_parse_spec names[] = {
      {  {"::"}                                  , {"::"}                                  , {nullptr}, {nullptr}}
      , {{"[::1]:99"}                            , {"::1"}                                 , {"99"}   , {nullptr}}
      , {{"127.0.0.1:8080"}                      , {"127.0.0.1"}                           , {"8080"} , {nullptr}}
      , {{"127.0.0.1:8080-Bob"}                  , {"127.0.0.1"}                           , {"8080"} , {"-Bob"}}
      , {{"127.0.0.1:"}                          , {"127.0.0.1"}                           , {nullptr}, {":"}}
      , {{"foo.example.com"}                     , {"foo.example.com"}                     , {nullptr}, {nullptr}}
      , {{"foo.example.com:99"}                  , {"foo.example.com"}                     , {"99"}   , {nullptr}}
      , {{"ffee::24c3:3349:3cee:0143"}           , {"ffee::24c3:3349:3cee:0143"}           , {nullptr}, {nullptr}}
      , {{"fe80:88b5:4a:20c:29ff:feae:1c33:8080"}, {"fe80:88b5:4a:20c:29ff:feae:1c33:8080"}, {nullptr}, {nullptr}}
      , {{"[ffee::24c3:3349:3cee:0143]"}         , {"ffee::24c3:3349:3cee:0143"}           , {nullptr}, {nullptr}}
      , {{"[ffee::24c3:3349:3cee:0143]:80"}      , {"ffee::24c3:3349:3cee:0143"}           , {"80"}   , {nullptr}}
      , {{"[ffee::24c3:3349:3cee:0143]:8080x"}   , {"ffee::24c3:3349:3cee:0143"}           , {"8080"} , {"x"}}
      ,};

  for (auto const&s : names) {
    std::string_view host, port, rest;

    REQUIRE(IPEndpoint::tokenize(s.hostspec, &host, &port, &rest) == true);
    REQUIRE(s.host == host);
    REQUIRE(s.port == port);
    REQUIRE(s.rest == rest);
  }

  IP4Addr alpha{"172.96.12.134"};
  CHECK(alpha == IP4Addr{"172.96.12.134"});
  CHECK(alpha == IP4Addr{IPAddr{"172.96.12.134"}});
  CHECK(alpha == IPAddr{IPEndpoint{"172.96.12.134:80"}});

  // Do a bit of IPv6 testing.
  IP6Addr a6_null;
  IP6Addr a6_1{"fe80:88b5:4a:20c:29ff:feae:5587:1c33"};
  IP6Addr a6_2{"fe80:88b5:4a:20c:29ff:feae:5587:1c34"};
  IP6Addr a6_3{"de80:88b5:4a:20c:29ff:feae:5587:1c35"};

  REQUIRE(a6_1 != a6_null);
  REQUIRE(a6_1 != a6_2);
  REQUIRE(a6_1 < a6_2);
  REQUIRE(a6_2 > a6_1);
  ++a6_1;
  REQUIRE(a6_1 == a6_2);
  ++a6_1;
  REQUIRE(a6_1 != a6_2);
  REQUIRE(a6_1 > a6_2);

  REQUIRE(a6_3 != a6_2);
  REQUIRE(a6_3 < a6_2);
  REQUIRE(a6_2 > a6_3);

  // Little bit of IP4 address arithmetic / comparison testing.
  IP4Addr a4_null;
  IP4Addr a4_1{"172.28.56.33"};
  IP4Addr a4_2{"172.28.56.34"};
  IP4Addr a4_3{"170.28.56.35"};
  IP4Addr a4_loopback{"127.0.0.1"_tv};
  IP4Addr ip4_loopback{INADDR_LOOPBACK};

  REQUIRE(a4_loopback == ip4_loopback);

  REQUIRE(a4_1 != a4_null);
  REQUIRE(a4_1 != a4_2);
  REQUIRE(a4_1 < a4_2);
  REQUIRE(a4_2 > a4_1);
  ++a4_1;
  REQUIRE(a4_1 == a4_2);
  ++a4_1;
  REQUIRE(a4_1 != a4_2);
  REQUIRE(a4_1 > a4_2);
  REQUIRE(a4_3 != a4_2);
  REQUIRE(a4_3 < a4_2);
  REQUIRE(a4_2 > a4_3);

  // For this data, the bytes should be in IPv6 network order.
  static const std::tuple<TextView, bool, IP6Addr::raw_type> ipv6_ex[] = {
      {  "::"                                , true , {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
      , {"::1"                               , true , {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}}
      , {":::"                               , false, {}}
      , {"fe80::20c:29ff:feae:5587:1c33"     , true , {0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x0C, 0x29, 0xFF, 0xFE, 0xAE, 0x55, 0x87, 0x1C, 0x33}}
      , {"fe80:20c:29ff:feae:5587::1c33"     , true , {0xFE, 0x80, 0x02, 0x0C, 0x29, 0xFF, 0xFE, 0xAE, 0x55, 0x87, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x33}}
      , {"fe80:20c:29ff:feae:5587:1c33::"    , true , {0xFE, 0x80, 0x02, 0x0C, 0x29, 0xFF, 0xFE, 0xAE, 0x55, 0x87, 0x1c, 0x33, 0x00, 0x00, 0x00, 0x00}}
      , {"::fe80:20c:29ff:feae:5587:1c33"    , true , {0x00, 0x00, 0x00, 0x00, 0xFE, 0x80, 0x02, 0x0C, 0x29, 0xFF, 0xFE, 0xAE, 0x55, 0x87, 0x1c, 0x33}}
      , {":fe80:20c:29ff:feae:5587:4A43:1c33", false, {}}
      , {"fe80:20c::29ff:feae:5587::1c33"    , false, {}}
  };

  for (auto const&item : ipv6_ex) {
    auto &&[text, result, data]{item};
    IP6Addr addr;
    REQUIRE(result == addr.load(text));
    if (result) {
      union {
        in6_addr _inet;
        IP6Addr::raw_type _raw;
      } ar;
      ar._inet = addr.network_order();
      REQUIRE(ar._raw == data);
    }
  }

  IP4Range r4;
  REQUIRE(r4.load("10.242.129.0-10.242.129.127") == true);
  REQUIRE(r4.min() == IP4Addr("10.242.129.0"));
  REQUIRE(r4.max() == IP4Addr("10.242.129.127"));
  REQUIRE(r4.load("10.242.129.0/25") == true);
  REQUIRE(r4.min() == IP4Addr("10.242.129.0"));
  REQUIRE(r4.max() == IP4Addr("10.242.129.127"));
  REQUIRE(r4.load("2.2.2.2") == true);
  REQUIRE(r4.min() == IP4Addr("2.2.2.2"));
  REQUIRE(r4.max() == IP4Addr("2.2.2.2"));
  REQUIRE(r4.load("2.2.2.2.2") == false);
  REQUIRE(r4.load("2.2.2.2-fe80:20c::29ff:feae:5587::1c33") == false);
};

TEST_CASE("IP Formatting", "[libswoc][ip][bwformat]") {
  IPEndpoint ep;
  std::string_view addr_1{"[ffee::24c3:3349:3cee:143]:8080"};
  std::string_view addr_2{"172.17.99.231:23995"};
  std::string_view addr_3{"[1337:ded:BEEF::]:53874"};
  std::string_view addr_4{"[1337::ded:BEEF]:53874"};
  std::string_view addr_5{"[1337:0:0:ded:BEEF:0:0:956]:53874"};
  std::string_view addr_6{"[1337:0:0:ded:BEEF:0:0:0]:53874"};
  std::string_view addr_7{"172.19.3.105:4951"};
  std::string_view addr_null{"[::]:53874"};
  std::string_view localhost{"[::1]:8080"};
  swoc::LocalBufferWriter<1024> w;

  REQUIRE(ep.parse(addr_null) == true);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "::");

  ep.set_to_loopback(AF_INET6);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "::1");

  REQUIRE(ep.parse(addr_1) == true);
  w.clear().print("{}", ep);
  REQUIRE(w.view() == addr_1);
  w.clear().print("{::p}", ep);
  REQUIRE(w.view() == "8080");
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == addr_1.substr(1, 24)); // check the brackets are dropped.
  w.clear().print("[{::a}]", ep);
  REQUIRE(w.view() == addr_1.substr(0, 26)); // check the brackets are dropped.
  w.clear().print("[{0::a}]:{0::p}", ep);
  REQUIRE(w.view() == addr_1); // check the brackets are dropped.
  w.clear().print("{::=a}", ep);
  REQUIRE(w.view() == "ffee:0000:0000:0000:24c3:3349:3cee:0143");
  w.clear().print("{:: =a}", ep);
  REQUIRE(w.view() == "ffee:   0:   0:   0:24c3:3349:3cee: 143");

  REQUIRE(ep.parse(addr_2) == true);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == addr_2.substr(0, 13));
  w.clear().print("{0::a}", ep);
  REQUIRE(w.view() == addr_2.substr(0, 13));
  w.clear().print("{::ap}", ep);
  REQUIRE(w.view() == addr_2);
  w.clear().print("{::f}", ep);
  REQUIRE(w.view() == "ipv4");
  w.clear().print("{::fpa}", ep);
  REQUIRE(w.view() == "172.17.99.231:23995 ipv4");
  w.clear().print("{0::a} .. {0::p}", ep);
  REQUIRE(w.view() == "172.17.99.231 .. 23995");
  w.clear().print("<+> {0::a} <+> {0::p}", ep);
  REQUIRE(w.view() == "<+> 172.17.99.231 <+> 23995");
  w.clear().print("<+> {0::a} <+> {0::p} <+>", ep);
  REQUIRE(w.view() == "<+> 172.17.99.231 <+> 23995 <+>");
  w.clear().print("{:: =a}", ep);
  REQUIRE(w.view() == "172. 17. 99.231");
  w.clear().print("{::=a}", ep);
  REQUIRE(w.view() == "172.017.099.231");

  REQUIRE(ep.parse(addr_3) == true);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "1337:ded:beef::"_tv);

  REQUIRE(ep.parse(addr_4) == true);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "1337::ded:beef"_tv);

  REQUIRE(ep.parse(addr_5) == true);
  w.clear().print("{:X:a}", ep);
  REQUIRE(w.view() == "1337::DED:BEEF:0:0:956");

  REQUIRE(ep.parse(addr_6) == true);
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "1337:0:0:ded:beef::");

  // Documentation examples
  REQUIRE(ep.parse(addr_7) == true);
  w.clear().print("To {}", ep);
  REQUIRE(w.view() == "To 172.19.3.105:4951");
  w.clear().print("To {0::a} on port {0::p}", ep); // no need to pass the argument twice.
  REQUIRE(w.view() == "To 172.19.3.105 on port 4951");
  w.clear().print("To {::=}", ep);
  REQUIRE(w.view() == "To 172.019.003.105:04951");
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == "172.19.3.105");
  w.clear().print("{::=a}", ep);
  REQUIRE(w.view() == "172.019.003.105");
  w.clear().print("{::0=a}", ep);
  REQUIRE(w.view() == "172.019.003.105");
  w.clear().print("{:: =a}", ep);
  REQUIRE(w.view() == "172. 19.  3.105");
  w.clear().print("{:>20:a}", ep);
  REQUIRE(w.view() == "        172.19.3.105");
  w.clear().print("{:>20:=a}", ep);
  REQUIRE(w.view() == "     172.019.003.105");
  w.clear().print("{:>20: =a}", ep);
  REQUIRE(w.view() == "     172. 19.  3.105");
  w.clear().print("{:<20:a}", ep);
  REQUIRE(w.view() == "172.19.3.105        ");

  REQUIRE(ep.parse(localhost) == true);
  w.clear().print("{}", ep);
  REQUIRE(w.view() == localhost);
  w.clear().print("{::p}", ep);
  REQUIRE(w.view() == "8080");
  w.clear().print("{::a}", ep);
  REQUIRE(w.view() == localhost.substr(1, 3)); // check the brackets are dropped.
  w.clear().print("[{::a}]", ep);
  REQUIRE(w.view() == localhost.substr(0, 5));
  w.clear().print("[{0::a}]:{0::p}", ep);
  REQUIRE(w.view() == localhost); // check the brackets are dropped.
  w.clear().print("{::=a}", ep);
  REQUIRE(w.view() == "0000:0000:0000:0000:0000:0000:0000:0001");
  w.clear().print("{:: =a}", ep);
  REQUIRE(w.view() == "   0:   0:   0:   0:   0:   0:   0:   1");
}

TEST_CASE("IP ranges and networks", "[libswoc][ip][net][range]") {
  swoc::IP4Range r_0;
  swoc::IP4Range r_1{"1.1.1.0-1.1.1.9"};
  swoc::IP4Range r_2{"1.1.2.0-1.1.2.97"};
  swoc::IP4Range r_3{"1.1.0.0-1.2.0.0"};
  swoc::IP4Range r_4{"10.33.45.19-10.33.45.76"};
  swoc::IP6Range r_5{
      "2001:1f2d:c587:24c3:9128:3349:3cee:143-ffee:1f2d:c587:24c3:9128:3349:3cFF:FFFF"_tv};

  CHECK(true == r_0.empty());
  CHECK(false == r_1.empty());

  swoc::IPMask mask{127};
  CHECK(r_5.min() == (r_5.min() | swoc::IPMask(128)));
  CHECK(r_5.min() == (r_5.min() | mask));
  CHECK(r_5.min() != (r_5.min() & mask));

  swoc::IP6Addr a_1{"2001:1f2d:c587:24c4::"};
  CHECK(a_1 == (a_1 & swoc::IPMask{62}));

  std::array<swoc::IP4Net, 7> r_4_nets =
      {{
           "10.33.45.19/32"_tv
           , "10.33.45.20/30"_tv
           , "10.33.45.24/29"_tv
           , "10.33.45.32/27"_tv
           , "10.33.45.64/29"_tv
           , "10.33.45.72/30"_tv
           , "10.33.45.76/32"_tv
       }};
  auto r4_net = r_4_nets.begin();
  for (auto const &net : r_4.networks()) {
    REQUIRE(r4_net != r_4_nets.end());
    CHECK(*r4_net == net);
    ++r4_net;
  }

  // Let's try that again, with @c IPRange instead.
  r4_net = r_4_nets.begin();
  for (auto const& net : IPRange{r_4}.networks()) {
    REQUIRE(r4_net != r_4_nets.end());
    CHECK(*r4_net == net);
    ++r4_net;
  }

  std::array<swoc::IP6Net, 130> r_5_nets =
      {{
           {IP6Addr{
               "2001:1f2d:c587:24c3:9128:3349:3cee:143"}, IPMask{
               128}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:144"}, IPMask{126}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:148"}, IPMask{125}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:150"}, IPMask{124}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:160"}, IPMask{123}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:180"}, IPMask{121}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:200"}, IPMask{119}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:400"}, IPMask{118}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:800"}, IPMask{117}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:1000"}, IPMask{116}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:2000"}, IPMask{115}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:4000"}, IPMask{114}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cee:8000"}, IPMask{113}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cef:0"}, IPMask{112}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3cf0:0"}, IPMask{108}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3d00:0"}, IPMask{104}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:3e00:0"}, IPMask{103}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:4000:0"}, IPMask{98}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3349:8000:0"}, IPMask{97}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:334a::"}, IPMask{95}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:334c::"}, IPMask{94}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3350::"}, IPMask{92}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3360::"}, IPMask{91}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3380::"}, IPMask{89}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3400::"}, IPMask{86}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:3800::"}, IPMask{85}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:4000::"}, IPMask{82}}
           , {IP6Addr{
              "2001:1f2d:c587:24c3:9128:8000::"}, IPMask{81}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9129::"}, IPMask{
              80}}
           , {IP6Addr{"2001:1f2d:c587:24c3:912a::"}, IPMask{
              79}}
           , {IP6Addr{"2001:1f2d:c587:24c3:912c::"}, IPMask{
              78}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9130::"}, IPMask{
              76}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9140::"}, IPMask{
              74}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9180::"}, IPMask{
              73}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9200::"}, IPMask{
              71}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9400::"}, IPMask{
              70}}
           , {IP6Addr{"2001:1f2d:c587:24c3:9800::"}, IPMask{
              69}}
           , {IP6Addr{"2001:1f2d:c587:24c3:a000::"}, IPMask{
              67}}
           , {IP6Addr{"2001:1f2d:c587:24c3:c000::"}, IPMask{
              66}}
           , {IP6Addr{"2001:1f2d:c587:24c4::"}, IPMask{62}}
           , {IP6Addr{"2001:1f2d:c587:24c8::"}, IPMask{61}}
           , {IP6Addr{"2001:1f2d:c587:24d0::"}, IPMask{60}}
           , {IP6Addr{"2001:1f2d:c587:24e0::"}, IPMask{59}}
           , {IP6Addr{"2001:1f2d:c587:2500::"}, IPMask{56}}
           , {IP6Addr{"2001:1f2d:c587:2600::"}, IPMask{55}}
           , {IP6Addr{"2001:1f2d:c587:2800::"}, IPMask{53}}
           , {IP6Addr{"2001:1f2d:c587:3000::"}, IPMask{52}}
           , {IP6Addr{"2001:1f2d:c587:4000::"}, IPMask{50}}
           , {IP6Addr{"2001:1f2d:c587:8000::"}, IPMask{49}}
           , {IP6Addr{"2001:1f2d:c588::"}, IPMask{45}}
           , {IP6Addr{"2001:1f2d:c590::"}, IPMask{44}}
           , {IP6Addr{"2001:1f2d:c5a0::"}, IPMask{43}}
           , {IP6Addr{"2001:1f2d:c5c0::"}, IPMask{42}}
           , {IP6Addr{"2001:1f2d:c600::"}, IPMask{39}}
           , {IP6Addr{"2001:1f2d:c800::"}, IPMask{37}}
           , {IP6Addr{"2001:1f2d:d000::"}, IPMask{36}}
           , {IP6Addr{"2001:1f2d:e000::"}, IPMask{35}}
           , {IP6Addr{"2001:1f2e::"}, IPMask{31}}
           , {IP6Addr{"2001:1f30::"}, IPMask{28}}
           , {IP6Addr{"2001:1f40::"}, IPMask{26}}
           , {IP6Addr{"2001:1f80::"}, IPMask{25}}
           , {IP6Addr{"2001:2000::"}, IPMask{19}}
           , {IP6Addr{"2001:4000::"}, IPMask{18}}
           , {IP6Addr{"2001:8000::"}, IPMask{17}}
           , {IP6Addr{"2002::"}, IPMask{15}}
           , {IP6Addr{"2004::"}, IPMask{14}}
           , {IP6Addr{"2008::"}, IPMask{13}}
           , {IP6Addr{"2010::"}, IPMask{12}}
           , {IP6Addr{"2020::"}, IPMask{11}}
           , {IP6Addr{"2040::"}, IPMask{10}}
           , {IP6Addr{"2080::"}, IPMask{9}}
           , {IP6Addr{"2100::"}, IPMask{8}}
           , {IP6Addr{"2200::"}, IPMask{7}}
           , {IP6Addr{"2400::"}, IPMask{6}}
           , {IP6Addr{"2800::"}, IPMask{5}}
           , {IP6Addr{"3000::"}, IPMask{4}}
           , {IP6Addr{"4000::"}, IPMask{2}}
           , {IP6Addr{"8000::"}, IPMask{2}}
           , {IP6Addr{"c000::"}, IPMask{3}}
           , {IP6Addr{"e000::"}, IPMask{4}}
           , {IP6Addr{"f000::"}, IPMask{5}}
           , {IP6Addr{"f800::"}, IPMask{6}}
           , {IP6Addr{"fc00::"}, IPMask{7}}
           , {IP6Addr{"fe00::"}, IPMask{8}}
           , {IP6Addr{"ff00::"}, IPMask{9}}
           , {IP6Addr{"ff80::"}, IPMask{10}}
           , {IP6Addr{"ffc0::"}, IPMask{11}}
           , {IP6Addr{"ffe0::"}, IPMask{13}}
           , {IP6Addr{"ffe8::"}, IPMask{14}}
           , {IP6Addr{"ffec::"}, IPMask{15}}
           , {IP6Addr{"ffee::"}, IPMask{20}}
           , {IP6Addr{"ffee:1000::"}, IPMask{21}}
           , {IP6Addr{"ffee:1800::"}, IPMask{22}}
           , {IP6Addr{"ffee:1c00::"}, IPMask{23}}
           , {IP6Addr{"ffee:1e00::"}, IPMask{24}}
           , {IP6Addr{"ffee:1f00::"}, IPMask{27}}
           , {IP6Addr{"ffee:1f20::"}, IPMask{29}}
           , {IP6Addr{"ffee:1f28::"}, IPMask{30}}
           , {IP6Addr{"ffee:1f2c::"}, IPMask{32}}
           , {IP6Addr{"ffee:1f2d::"}, IPMask{33}}
           , {IP6Addr{"ffee:1f2d:8000::"}, IPMask{34}}
           , {IP6Addr{"ffee:1f2d:c000::"}, IPMask{38}}
           , {IP6Addr{"ffee:1f2d:c400::"}, IPMask{40}}
           , {IP6Addr{"ffee:1f2d:c500::"}, IPMask{41}}
           , {IP6Addr{"ffee:1f2d:c580::"}, IPMask{46}}
           , {IP6Addr{"ffee:1f2d:c584::"}, IPMask{47}}
           , {IP6Addr{"ffee:1f2d:c586::"}, IPMask{48}}
           , {IP6Addr{"ffee:1f2d:c587::"}, IPMask{51}}
           , {IP6Addr{"ffee:1f2d:c587:2000::"}, IPMask{54}}
           , {IP6Addr{"ffee:1f2d:c587:2400::"}, IPMask{57}}
           , {IP6Addr{"ffee:1f2d:c587:2480::"}, IPMask{58}}
           , {IP6Addr{"ffee:1f2d:c587:24c0::"}, IPMask{63}}
           , {IP6Addr{"ffee:1f2d:c587:24c2::"}, IPMask{64}}
           , {IP6Addr{"ffee:1f2d:c587:24c3::"}, IPMask{65}}
           , {IP6Addr{"ffee:1f2d:c587:24c3:8000::"}, IPMask{
              68}}
           , {IP6Addr{"ffee:1f2d:c587:24c3:9000::"}, IPMask{
              72}}
           , {IP6Addr{"ffee:1f2d:c587:24c3:9100::"}, IPMask{
              75}}
           , {IP6Addr{"ffee:1f2d:c587:24c3:9120::"}, IPMask{
              77}}
           , {IP6Addr{"ffee:1f2d:c587:24c3:9128::"}, IPMask{
              83}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:2000::"}, IPMask{84}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3000::"}, IPMask{87}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3200::"}, IPMask{88}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3300::"}, IPMask{90}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3340::"}, IPMask{93}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3348::"}, IPMask{96}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3349::"}, IPMask{99}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3349:2000:0"}, IPMask{100}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3349:3000:0"}, IPMask{101}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3349:3800:0"}, IPMask{102}}
           , {IP6Addr{
              "ffee:1f2d:c587:24c3:9128:3349:3c00:0"}, IPMask{104}}
       }};

  auto r5_net = r_5_nets.begin();
  for (auto const&[addr, mask] : r_5.networks()) {
    REQUIRE(r5_net != r_5_nets.end());
    CHECK(*r5_net == swoc::IP6Net{addr, mask});
    ++r5_net;
  }

  // Try it again, using @c IPRange.
  r5_net = r_5_nets.begin();
  for ( auto const&[addr, mask] : IPRange{r_5}.networks()) {
    REQUIRE(r5_net != r_5_nets.end());
    CHECK(*r5_net == swoc::IPNet{addr, mask});
    ++r5_net;
  }
}

TEST_CASE("IP Space Int", "[libswoc][ip][ipspace]") {
  using int_space = swoc::IPSpace<unsigned>;
  int_space space;

  REQUIRE(space.count() == 0);

  space.mark(IPRange{{IP4Addr("172.16.0.0"), IP4Addr("172.16.0.255")}}, 1);
  auto result = space.find(IPAddr{"172.16.0.97"});
  REQUIRE(result != nullptr);
  REQUIRE(*result == 1);

  result = space.find(IPAddr{"172.17.0.97"});
  REQUIRE(result == nullptr);

  space.mark(IPRange{"172.16.0.12-172.16.0.25"_tv}, 2);

  result = space.find(IPAddr{"172.16.0.21"});
  REQUIRE(result != nullptr);
  REQUIRE(*result == 2);
  REQUIRE(space.count() == 3);

  space.clear();
  auto BF = [](unsigned&lhs, unsigned rhs) -> bool {
    lhs |= rhs;
    return true;
  };
  unsigned *payload;
  swoc::IP4Range r_1{"1.1.1.0-1.1.1.9"};
  swoc::IP4Range r_2{"1.1.2.0-1.1.2.97"};
  swoc::IP4Range r_3{"1.1.0.0-1.2.0.0"};

  // Compiler check - make sure both of these work.
  REQUIRE(r_1.min() == IP4Addr("1.1.1.0"_tv));
  REQUIRE(r_1.max() == IPAddr("1.1.1.9"_tv));

  space.blend(r_1, 0x1, BF);
  REQUIRE(space.count() == 1);
  REQUIRE(nullptr == space.find(r_2.min()));
  REQUIRE(nullptr != space.find(r_1.min()));
  REQUIRE(nullptr != space.find(r_1.max()));
  REQUIRE(nullptr != space.find(IP4Addr{"1.1.1.7"}));
  CHECK(0x1 == *space.find(IP4Addr{"1.1.1.7"}));

  space.blend(r_2, 0x2, BF);
  REQUIRE(space.count() == 2);
  REQUIRE(nullptr != space.find(r_1.min()));
  payload = space.find(r_2.min());
  REQUIRE(payload != nullptr);
  REQUIRE(*payload == 0x2);
  payload = space.find(r_2.max());
  REQUIRE(payload != nullptr);
  REQUIRE(*payload == 0x2);

  space.blend(r_3, 0x4, BF);
  REQUIRE(space.count() == 5);
  payload = space.find(r_2.min());
  REQUIRE(payload != nullptr);
  REQUIRE(*payload == 0x6);

  payload = space.find(r_3.min());
  REQUIRE(payload != nullptr);
  REQUIRE(*payload == 0x4);

  payload = space.find(r_1.max());
  REQUIRE(payload != nullptr);
  REQUIRE(*payload == 0x5);

  space.blend(IPRange{r_2.min(), r_3.max()}, 0x6, BF);
  REQUIRE(space.count() == 4);

  std::array<std::tuple<TextView, int>, 9> ranges = {
      {
          {"100.0.0.0-100.0.0.255", 0}
          , {"100.0.1.0-100.0.1.255", 1}
          , {"100.0.2.0-100.0.2.255", 2}
          , {"100.0.3.0-100.0.3.255", 3}
          , {"100.0.4.0-100.0.4.255", 4}
          , {"100.0.5.0-100.0.5.255", 5}
          , {"100.0.6.0-100.0.6.255", 6}
          , {"100.0.0.0-100.0.0.255", 31}
          , {"100.0.1.0-100.0.1.255", 30}
      }};

  space.clear();
  for (auto &&[text, value] : ranges) {
    IPRange range{text};
    space.mark(IPRange{text}, value);
  }

  CHECK(7 == space.count());
  CHECK(nullptr != space.find(IP4Addr{"100.0.4.16"}));
  CHECK(nullptr != space.find(IPAddr{"100.0.4.16"}));
  CHECK(nullptr != space.find(IPAddr{IPEndpoint{"100.0.4.16:80"}}));
}

TEST_CASE("IPSpace bitset", "[libswoc][ipspace][bitset]") {
  using PAYLOAD = std::bitset<32>;
  using Space = swoc::IPSpace<PAYLOAD>;

  std::array<std::tuple<TextView, std::initializer_list<unsigned>>, 6> ranges = {
      {
          {"172.28.56.12-172.28.56.99"_tv, {0, 2, 3}}
          , {"10.10.35.0/24"_tv, {1, 2}}
          , {"192.168.56.0/25"_tv, {10, 12, 31}}
          , {"1337::ded:beef-1337::ded:ceef"_tv, {4, 5, 6, 7}}
          , {"ffee:1f2d:c587:24c3:9128:3349:3cee:143-ffee:1f2d:c587:24c3:9128:3349:3cFF:FFFF"_tv, {9, 10, 18}}
          , {"10.12.148.0/23"_tv, {1, 2, 17}}
      }};

  Space space;

  for (auto &&[text, bit_list] : ranges) {
    PAYLOAD bits;
    for (auto bit : bit_list) {
      bits[bit] = true;
    }
    space.mark(IPRange{text}, bits);
  }
  REQUIRE(space.count() == ranges.size());
}

TEST_CASE("IPSpace docJJ", "[libswoc][ipspace][docJJ]") {
  using PAYLOAD = std::bitset<32>;
  using Space = swoc::IPSpace<PAYLOAD>;
  auto blender = [](PAYLOAD&lhs, PAYLOAD const&rhs) -> bool {
    lhs |= rhs;
    return true;
  };
  auto make_bits = [](std::initializer_list<unsigned> idx) -> PAYLOAD {
    PAYLOAD bits;
    for (auto bit : idx) {
      bits[bit] = true;
    }
    return bits;
  };

  std::array<std::tuple<TextView, PAYLOAD>, 9> ranges = {
      {
          {"100.0.0.0-100.0.0.255", make_bits({0})}
          , {"100.0.1.0-100.0.1.255", make_bits({1})}
          , {"100.0.2.0-100.0.2.255", make_bits({2})}
          , {"100.0.3.0-100.0.3.255", make_bits({3})}
          , {"100.0.4.0-100.0.4.255", make_bits({4})}
          , {"100.0.5.0-100.0.5.255", make_bits({5})}
          , {"100.0.6.0-100.0.6.255", make_bits({6})}
          , {"100.0.0.0-100.0.0.255", make_bits({31})}
          , {"100.0.1.0-100.0.1.255", make_bits({30})}
      }};

  std::array<std::initializer_list<unsigned>, 7> results = {{
                                                                {0, 31}
                                                                , {1, 30}
                                                                , {2}
                                                                , {3}
                                                                , {4}
                                                                , {5}
                                                                , {6}
                                                            }};

  Space space;

  for (auto && [text, bit_list] : ranges) {
    std::cout << W().print("{} = {}\n", text, bit_list);
    space.blend(IPRange{text}, bit_list, blender);
  }

  // Check iteration - verify forward and reverse iteration yield the correct number of ranges
  // and the range payloads match what is expected.
  REQUIRE(space.count() == results.size());
  unsigned idx;

  idx = 0;
  for (auto const&[range, bits] : space) {
    REQUIRE(idx < results.size());
    CHECK(bits == make_bits(results[idx]));
    ++idx;
  }

  idx = results.size();
  for (auto spot = space.end(); spot != space.begin();) {
    auto const&[range, bits]{*--spot};
    REQUIRE(idx > 0);
    --idx;
    CHECK(bits == make_bits(results[idx]));
  }

  // Check iterator copying.
  idx = 0;
  Space::iterator iter;
  for (auto spot = space.begin(); spot != space.end() ; ++spot) {
    iter = spot;
    auto const& [range, bits]{*iter};
    REQUIRE(idx < results.size());
    CHECK(bits == make_bits(results[idx]));
    ++idx;
  }
}

#if 0
TEST_CASE("IP Space YNETDB", "[libswoc][ipspace][ynetdb]") {
  std::set<std::string_view> Locations;
  std::set<std::string_view> Owners;
  std::set<std::string_view> Descriptions;
  swoc::MemArena arena;
  auto Localize = [&](TextView const&view) -> TextView {
    auto span = arena.alloc(view.size() + 1).rebind<char>();
    memcpy(span, view);
    span[view.size()] = '\0';
    return span.view();
  };

  struct Payload {
    std::string_view _colo;
    std::string_view _owner;
    std::string_view _descr;
    unsigned int _internal_p : 1;
    unsigned int _prod_p : 1;
    unsigned int _corp_p : 1;
    unsigned int _flakey_p : 1;
    unsigned int _secure_p : 1;

    Payload() {
      _internal_p = _prod_p = _corp_p = _flakey_p = _secure_p = false;
    }

    bool operator==(Payload const&that) {
      return
          this->_colo == that._colo &&
          this->_owner == that._owner &&
          this->_descr == that._descr &&
          this->_corp_p == that._corp_p &&
          this->_internal_p == that._internal_p &&
          this->_prod_p == that._prod_p &&
          this->_flakey_p == that._flakey_p &&
          this->_secure_p == that._secure_p;
    }
  };

  using Space = swoc::IPSpace<Payload>;

  Space space;

  std::error_code ec;
  swoc::file::path csv{"/tmp/ynetdb.csv"};

  // Force these so I can easily change the set of tests.
  auto start            = std::chrono::high_resolution_clock::now();
  auto delta = std::chrono::high_resolution_clock::now() - start;

  auto file_content{swoc::file::load(csv, ec)};
  TextView content{file_content};

  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "File load " << delta.count() << "ns or " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()
            << "ms" << std::endl;

  start = std::chrono::high_resolution_clock::now();
  while (content) {
    Payload payload;

    auto line = content.take_prefix_at('\n');
    if (line.trim_if(&isspace).empty() || *line == '#') {
      continue;
    }

    auto range_token{line.take_prefix_at(',')};
    auto internal_token{line.take_prefix_at(',')};
    auto owner_token{line.take_prefix_at(',')};
    auto colo_token{line.take_prefix_at(',')};
    auto flags_token{line.take_prefix_at(',')};
    auto descr_token{line.take_prefix_at(',')};

    if (auto spot{Owners.find(owner_token)}; spot != Owners.end()) {
      payload._owner = *spot;
    } else {
      payload._owner = Localize(owner_token);
      Owners.insert(payload._owner);
    }

    if (auto spot{Descriptions.find(owner_token)}; spot != Descriptions.end()) {
      payload._descr = *spot;
    } else {
      payload._descr = Localize(owner_token);
      Descriptions.insert(payload._descr);
    }

    if (auto spot{Locations.find(owner_token)}; spot != Locations.end()) {
      payload._colo = *spot;
    } else {
      payload._colo = Localize(owner_token);
      Locations.insert(payload._colo);
    }

    static constexpr TextView INTERNAL_TAG{"yahoo"};
    static constexpr TextView FLAKEY_TAG{"flakey"};
    static constexpr TextView PROD_TAG{"prod"};
    static constexpr TextView CORP_TAG{"corp"};
    static constexpr TextView SECURE_TAG { "secure" };

    if (0 == strcasecmp(internal_token, INTERNAL_TAG)) {
      payload._internal_p = true;
    }

    if (flags_token != "-"_sv) {
      while (flags_token) {
        auto key = flags_token.take_prefix_at(';').trim_if(&isspace);
        if (0 == strcasecmp(key, FLAKEY_TAG)) {
          payload._flakey_p = true;
        } else if (0 == strcasecmp(key, PROD_TAG)) {
          payload._prod_p = true;
        } else if (0 == strcasecmp(key, CORP_TAG)) {
          payload._corp_p = true;
        } else if (0 == strcasecmp(key, SECURE_TAG)) {
          payload._secure_p = true;
        } else {
          throw std::domain_error("Bad flag tag");
        }
      }
    }

    auto BF = [](Payload&lhs, Payload const&rhs) -> bool {
      if (! lhs._colo.empty()) {
        std::cout << "Overlap " << lhs._descr << " with " << rhs._descr << std::endl;
      }
      return true;
    };
    swoc::LocalBufferWriter<1024> bw;
    swoc::IPRange range;
    if (range.load(range_token)) {
      space.blend(range, payload, BF);
    } else {
      std::cout << "Failed to parse range " << range_token << std::endl;
    }
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "Space load " << delta.count() << "ns or " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()
            << "ms" << std::endl;

  static constexpr size_t N_LOOPS = 1000000;
  start = std::chrono::high_resolution_clock::now();
  for ( size_t idx = 0 ; idx < N_LOOPS ; ++idx ) {
    space.find(IP4Addr(static_cast<in_addr_t>(idx * 2500)));
  }
  delta = std::chrono::high_resolution_clock::now() - start;
  std::cout << "Space IP4 lookup " << delta.count() << "ns or " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count()
            << "ms" << std::endl;

}
#endif
