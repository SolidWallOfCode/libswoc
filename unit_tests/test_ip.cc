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
#include <swoc/swoc_file.h>

using namespace std::literals;
using namespace swoc::literals;
using swoc::TextView;
using swoc::IPEndpoint;
using swoc::IP4Addr;
using swoc::IP4Range;
using swoc::IP6Addr;
using swoc::IP6Range;

TEST_CASE("ink_inet", "[libswoc][ip]") {
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

  // Do a bit of IPv6 testing.
  IP6Addr a6_null;
  IP6Addr a6_1{"fe80:88b5:4a:20c:29ff:feae:5587:1c33"};
  IP6Addr a6_2{"fe80:88b5:4a:20c:29ff:feae:5587:1c34"};
  IP6Addr a6_3{"de80:88b5:4a:20c:29ff:feae:5587:1c35"};
  IP6Addr a6_4{"fe80:88b5:4a:20c:29ff:feae:5587:1c34"};

  REQUIRE(a6_1 != a6_null);
  REQUIRE(a6_1 != a6_2);
  REQUIRE(a6_1 < a6_2);
  REQUIRE(a6_2 > a6_1);
  ++a6_1;
  REQUIRE(a6_1 == a6_2);
  ++a6_1;
  REQUIRE(a6_1 != a6_2);
  REQUIRE(a6_1 > a6_2);
  REQUIRE(a6_3 != a6_4);
  REQUIRE(a6_3 < a6_4);
  REQUIRE(a6_4 > a6_3);

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

  REQUIRE(ep.parse(addr_1) == true);
  w.print("{}", ep);
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

#if 0
  ep.setToLoopback(AF_INET6);
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "::1");
  REQUIRE(0 == ats_ip_pton(addr_3, &ep.sa));
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "1337:ded:beef::");
  REQUIRE(0 == ats_ip_pton(addr_4, &ep.sa));
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "1337::ded:beef");

  REQUIRE(0 == ats_ip_pton(addr_5, &ep.sa));
  w.reset().print("{:X:a}", ep);
  REQUIRE(w.view() == "1337::DED:BEEF:0:0:956");

  REQUIRE(0 == ats_ip_pton(addr_6, &ep.sa));
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "1337:0:0:ded:beef::");

  REQUIRE(0 == ats_ip_pton(addr_null, &ep.sa));
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "::");
#endif

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

#if 0
  // Documentation examples
  REQUIRE(0 == ats_ip_pton(addr_7, &ep.sa));
  w.reset().print("To {}", ep);
  REQUIRE(w.view() == "To 172.19.3.105:4951");
  w.reset().print("To {0::a} on port {0::p}", ep); // no need to pass the argument twice.
  REQUIRE(w.view() == "To 172.19.3.105 on port 4951");
  w.reset().print("To {::=}", ep);
  REQUIRE(w.view() == "To 172.019.003.105:04951");
  w.reset().print("{::a}", ep);
  REQUIRE(w.view() == "172.19.3.105");
  w.reset().print("{::=a}", ep);
  REQUIRE(w.view() == "172.019.003.105");
  w.reset().print("{::0=a}", ep);
  REQUIRE(w.view() == "172.019.003.105");
  w.reset().print("{:: =a}", ep);
  REQUIRE(w.view() == "172. 19.  3.105");
  w.reset().print("{:>20:a}", ep);
  REQUIRE(w.view() == "        172.19.3.105");
  w.reset().print("{:>20:=a}", ep);
  REQUIRE(w.view() == "     172.019.003.105");
  w.reset().print("{:>20: =a}", ep);
  REQUIRE(w.view() == "     172. 19.  3.105");
  w.reset().print("{:<20:a}", ep);
  REQUIRE(w.view() == "172.19.3.105        ");

  w.reset().print("{:p}", reinterpret_cast<sockaddr const *>(0x1337beef));
  REQUIRE(w.view() == "0x1337beef");

  ats_ip_pton(addr_1, &ep.sa);
  w.reset().print("{}", swoc::bwf::Hex_Dump(ep));
  REQUIRE(w.view() == "ffee00000000000024c333493cee0143");
#endif

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

// Property maps for IPSpace.

/** A @c Property is a collection of names which are values of the property.
 *
 */
class Property {
  using self_type = Property;
public:
  Property(unsigned idx) : _idx(idx) {}

  unsigned operator[](std::string_view const&);

protected:
  unsigned _idx;
  std::vector<std::string> _names;
};

unsigned Property::operator[](std::string_view const&name) {
  if (auto spot = std::find_if(_names.begin(), _names.end()
                               , [&](std::string const&prop) { return strcasecmp(name, prop); });
      spot != _names.end()) {
    return spot - _names.begin();
  }
  // Create a value
  _names.emplace_back(name);
  return _names.size() - 1;
}

class PropertyGroup {
  using self_type = PropertyGroup;
public:
  Property *operator[](std::string_view const&name);

protected:
  using Item = std::tuple<std::string, std::unique_ptr<Property>>;
  std::vector<Item> _properties;
};


Property *PropertyGroup::operator[](std::string_view const&name) {
  if (auto spot = std::find_if(_properties.begin(), _properties.end()
                               , [&](
            Item const&item) { return strcasecmp(name, std::get<0>(item)); });
      spot != _properties.end()) {
    return std::get<1>(*spot).get();
  }
  // Create a new property.
  _properties.emplace_back(name, new Property(_properties.size()));
  return std::get<1>(_properties.back()).get();
}

TEST_CASE("IP Space Int", "[libswoc][ip][ipspace]") {
  using int_space = swoc::IPSpace<unsigned>;
  int_space space;
  auto dump = [] (int_space & space) -> void {
    swoc::LocalBufferWriter<1024> w;
    std::cout << "Dumping " << space.count() << " ranges" << std::endl;
    for ( auto & r : space ) {
      std::cout << w.clear().print("{} - {} : {}\n", r.min(), r.max(), r.payload()).view();
    }
  };

  REQUIRE(space.count() == 0);

  space.mark({IP4Addr("172.16.0.0"), IP4Addr("172.16.0.255")}, 1);
  auto result = space.find({"172.16.0.97"});
  REQUIRE(result != nullptr);
  REQUIRE(*result == 1);

  result = space.find({"172.17.0.97"});
  REQUIRE(result == nullptr);

  space.mark({IP4Addr("172.16.0.12"), IP4Addr("172.16.0.25")}, 2);

  result = space.find({"172.16.0.21"});
  REQUIRE(result != nullptr);
  REQUIRE(*result == 2);
  REQUIRE(space.count() == 3);

  space.clear();
  auto BF = [](unsigned&lhs, unsigned rhs) -> bool { lhs |= rhs; return true; };
  unsigned *payload;
  swoc::IP4Range r_1{"1.1.1.0-1.1.1.9"};
  swoc::IP4Range r_2{"1.1.2.0-1.1.2.97"};
  swoc::IP4Range r_3{"1.1.0.0-1.2.0.0"};

  space.blend(r_1, 0x1, BF);
  REQUIRE(space.count() == 1);
  REQUIRE(nullptr == space.find(r_2.min()));
  REQUIRE(nullptr != space.find(r_1.min()));
  REQUIRE(nullptr != space.find(r_1.max()));

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
  dump(space);
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

  space.blend({r_2.min(), r_3.max()}, 0x6, BF);
  dump(space);
  REQUIRE(space.count() == 4);
}

#if 1
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
