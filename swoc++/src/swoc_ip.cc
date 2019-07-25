/** @file

  IP address support.

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements.
  See the NOTICE file distributed with this work for additional information regarding copyright
  ownership.  The ASF licenses this file to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance with the License.  You may obtain a
  copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software distributed under the License
  is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
  or implied. See the License for the specific language governing permissions and limitations under
  the License.
 */

#include <swoc/swoc_ip.h>
#include "swoc/swoc_ip.h"
#include "swoc/swoc_meta.h"

using swoc::TextView;
using swoc::svtoi;
using swoc::svtou;
using namespace swoc::literals;

namespace
{
// Handle the @c sin_len member, the presence of which varies across compilation environments.
template <typename T>
auto
Set_Sockaddr_Len_Case(T *, swoc::meta::CaseTag<0>) -> decltype(swoc::meta::TypeFunc<void>())
{
}

template <typename T>
auto
Set_Sockaddr_Len_Case(T *addr, swoc::meta::CaseTag<1>) -> decltype(T::sin_len, swoc::meta::TypeFunc<void>())
{
  addr->sin_len = sizeof(T);
}

template <typename T>
void
Set_Sockaddr_Len(T *addr)
{
  Set_Sockaddr_Len_Case(addr, swoc::meta::CaseArg);
}

} // namespace

namespace swoc {
IPAddr const IPAddr::INVALID;
IP4Addr const IP4Addr::MIN{INADDR_ANY};
IP4Addr const IP4Addr::MAX{INADDR_BROADCAST};
IP6Addr const IP6Addr::MIN{0, 0};
IP6Addr const IP6Addr::MAX{std::numeric_limits<uint64_t>::max()
                           , std::numeric_limits<uint64_t>::max()};

bool
IPEndpoint::assign(sockaddr *dst, sockaddr const *src) {
  size_t n = 0;
  if (dst != src) {
    self_type::invalidate(dst);
    switch (src->sa_family) {
      case AF_INET:n = sizeof(sockaddr_in);
        break;
      case AF_INET6:n = sizeof(sockaddr_in6);
        break;
    }
    if (n) {
      memcpy(dst, src, n);
    }
  }
  return n != 0;
}

IPEndpoint&
IPEndpoint::assign(IPAddr const&src, in_port_t port) {
  switch (src.family()) {
    case AF_INET: {
      memset(&sa4, 0, sizeof sa4);
      sa4.sin_family = AF_INET;
      sa4.sin_addr.s_addr = src.network_ip4();
      sa4.sin_port = port;
      Set_Sockaddr_Len(&sa4);
    }
      break;
    case AF_INET6: {
      memset(&sa6, 0, sizeof sa6);
      sa6.sin6_family = AF_INET6;
      sa6.sin6_addr = src.network_ip6();
      sa6.sin6_port = port;
      Set_Sockaddr_Len(&sa6);
    }
      break;
  }
  return *this;
}

IPAddr&
IPAddr::assign(sockaddr const *addr) {
  if (addr) {
    switch (addr->sa_family) {
      case AF_INET:return this->assign(reinterpret_cast<sockaddr_in const *>(addr)->sin_addr.s_addr);
      case AF_INET6:return this->assign(reinterpret_cast<sockaddr_in6 const *>(addr)->sin6_addr);
      default:break;
    }
  }
  _family = AF_UNSPEC;
  return *this;
}

bool
IPEndpoint::tokenize(std::string_view str, std::string_view *addr, std::string_view *port
                     , std::string_view *rest) {
  TextView src(str); /// Easier to work with for parsing.
  // In case the incoming arguments are null, set them here and only check for null once.
  // it doesn't matter if it's all the same, the results will be thrown away.
  std::string_view local;
  if (!addr) {
    addr = &local;
  }
  if (!port) {
    port = &local;
  }
  if (!rest) {
    rest = &local;
  }

  *addr = std::string_view{};
  *port = std::string_view{};
  *rest = std::string_view{};

  // Let's see if we can find out what's in the address string.
  if (src) {
    bool colon_p = false;
    src.ltrim_if(&isspace);
    // Check for brackets.
    if ('[' == *src) {
      ++src; // skip bracket.
      *addr = src.take_prefix_at(']');
      if (':' == *src) {
        colon_p = true;
        ++src;
      }
    } else {
      TextView::size_type last = src.rfind(':');
      if (last != TextView::npos && last == src.find(':')) {
        // Exactly one colon - leave post colon stuff in @a src.
        *addr = src.take_prefix(last);
        colon_p = true;
      } else { // presume no port, use everything.
        *addr = src;
        src.clear();
      }
    }
    if (colon_p) {
      TextView tmp{src};
      src.ltrim_if(&isdigit);

      if (tmp.data() == src.data()) {               // no digits at all
        src.assign(tmp.data() - 1, tmp.size() + 1); // back up to include colon
      } else {
        *port = std::string_view(tmp.data(), src.data() - tmp.data());
      }
    }
    *rest = src;
  }
  return !addr->empty(); // true if we found an address.
}

bool
IPEndpoint::parse(std::string_view const&str) {
  TextView addr_str, port_str, rest;
  TextView src{TextView{str}.trim_if(&isspace)};

  if (this->tokenize(src, &addr_str, &port_str, &rest)) {
    if (rest.empty()) {
      IPAddr addr;
      if (addr.load(addr_str)) {
        auto n{swoc::svto_radix<10>(port_str)};
        if (port_str.empty() && 0 < n && n <= std::numeric_limits<in_port_t>::max()) {
          this->assign(addr, htons(n));
          return true;
        }
      }
    }
  }
  return false;
}

sockaddr *
IPAddr::fill(sockaddr *sa, in_port_t port) const {
  switch (sa->sa_family = _family) {
    case AF_INET: {
      sockaddr_in *sa4{reinterpret_cast<sockaddr_in *>(sa)};
      memset(sa4, 0, sizeof(*sa4));
      sa4->sin_addr.s_addr = _addr._ip4;
      sa4->sin_port = port;
      Set_Sockaddr_Len(sa4);
    }
      break;
    case AF_INET6: {
      sockaddr_in6 *sa6{reinterpret_cast<sockaddr_in6 *>(sa)};
      memset(sa6, 0, sizeof(*sa6));
      sa6->sin6_port = port;
      Set_Sockaddr_Len(sa6);
    }
      break;
  }
  return sa;
}

socklen_t
IPEndpoint::size() const {
  switch (sa.sa_family) {
    case AF_INET:return sizeof(sockaddr_in);
    case AF_INET6:return sizeof(sockaddr_in6);
    default:return sizeof(sockaddr);
  }
}

std::string_view
IPEndpoint::family_name(uint16_t family) {
  switch (family) {
    case AF_INET:return "ipv4"_sv;
    case AF_INET6:return "ipv6"_sv;
    case AF_UNIX:return "unix"_sv;
    case AF_UNSPEC:return "unspec"_sv;
  }
  return "unknown"_sv;
}

IPEndpoint&
IPEndpoint::set_to_any(int family) {
  memset(*this, 0, sizeof(*this));
  if (AF_INET == family) {
    sa4.sin_family = family;
    sa4.sin_addr.s_addr = INADDR_ANY;
    Set_Sockaddr_Len(&sa4);
  } else if (AF_INET6 == family) {
    sa6.sin6_family = family;
    sa6.sin6_addr = in6addr_any;
    Set_Sockaddr_Len(&sa6);
  }
  return *this;
}

IPEndpoint&
IPEndpoint::set_to_loopback(int family) {
  memset(*this, 0, sizeof(*this));
  if (AF_INET == family) {
    sa.sa_family = family;
    sa4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Set_Sockaddr_Len(&sa4);
  } else if (AF_INET6 == family) {
    static const in6_addr init = IN6ADDR_LOOPBACK_INIT;
    sa.sa_family = family;
    sa6.sin6_addr = init;
    Set_Sockaddr_Len(&sa6);
  }
  return *this;
}

bool
IP4Addr::load(std::string_view const&text) {
  TextView src{text};
  int n = SIZE; /// # of octets

  if (src.empty() || ('[' == *src && ((++src).empty() || src.back() != ']'))) {
    return false;
  }

  auto octet = reinterpret_cast<uint8_t *>(&_addr);
  while (n > 0 && !src.empty()) {
    TextView token{src.take_prefix_at('.')};
    auto x = svto_radix<10>(token);
    if (token.empty() && 0 <= x && x <= std::numeric_limits<uint8_t>::max()) {
      octet[--n] = x;
    } else {
      break;
    }
  }

  if (n == 0 && src.empty()) {
    return true;
  }
  _addr = INADDR_ANY;
  return false;
}

bool
IP6Addr::load(std::string_view const&str) {
  TextView src{str};
  int n = 0;
  int empty_idx = -1;
  auto quad = _addr._quad.data();

  if (src && '[' == *src) {
    ++src;
    if (src.empty() || src.back() != ']') {
      return false;
    }
    src.remove_suffix(1);
  }

  if (src.size() < 2) {
    return false;
  }

  // If the first character is ':' then it is an error for it not to be followed by another ':'.
  // Special case the empty address and loopback, otherwise just make a note of the leading '::'.
  if (src[0] == ':') {
    if (src[1] == ':') {
      if (src.size() == 2) {
        this->clear();
        return true;
      } else if (src.size() == 3 && src[2] == '1') {
        _addr._u64[0] = 0;
        _addr._u64[1] = 1;
        return true;
      } else {
        empty_idx = n;
        src.remove_prefix(2);
      }
    } else {
      return false;
    }
  }

  // Sadly the empty quads can't be done in line because it's not possible to know the correct index
  // of the next present quad until the entire address has been parsed.
  while (n < N_QUADS && !src.empty()) {
    TextView token{src.take_prefix_at(':')};
    if (token.empty()) {
      if (empty_idx >= 0) { // two instances of "::", fail.
        return false;
      }
      empty_idx = n;
    } else {
      TextView r;
      auto x = svtoi(token, &r, 16);
      if (r.size() == token.size()) {
        quad[QUAD_IDX[n++]] = x;
      } else {
        break;
      }
    }
  }

  // Handle empty quads - invalid if empty and still had a full set of quads
  if (empty_idx >= 0) {
    if (n < N_QUADS) {
      auto nil_idx = N_QUADS - (n - empty_idx);
      auto delta = N_QUADS - n;
      for (int k = N_QUADS - 1; k >= empty_idx; --k) {
        quad[QUAD_IDX[k]] = (k >= nil_idx ? quad[QUAD_IDX[k - delta]] : 0);
      }
      n = N_QUADS; // Mark success.
    } else {
      return false; // too many quads - full set plus empty.
    }
  }

  if (n == N_QUADS && src.empty()) {
    return true;
  }

  this->clear();
  return false;
}

// These are Intel correct, at some point will need to be architecture dependent.

void IP6Addr::reorder(in6_addr&dst, raw_type const&src) {
  self_type::reorder(dst.s6_addr, src.data());
  self_type::reorder(dst.s6_addr + WORD_SIZE, src.data() + WORD_SIZE);
}

void IP6Addr::reorder(raw_type&dst, in6_addr const&src) {
  self_type::reorder(dst.data(), src.s6_addr);
  self_type::reorder(dst.data() + WORD_SIZE, src.s6_addr + WORD_SIZE);
}

bool
IPAddr::load(const std::string_view&text) {
  TextView src{text};
  src.ltrim_if(&isspace);

  if (TextView::npos != src.prefix(5).find_first_of('.')) {
    _family = AF_INET;
  } else if (TextView::npos != src.prefix(6).find_first_of(':')) {
    _family = AF_INET6;
  } else {
    _family = AF_UNSPEC;
  }

  // Do the real parse now
  switch (_family) {
    case AF_INET:
      if (!_addr._ip4.load(src)) {
        _family = AF_UNSPEC;
      }
      break;
    case AF_INET6:
      if (!_addr._ip6.load(src)) {
        _family = AF_UNSPEC;
      }
      break;
  }
  return this->is_valid();
}

#if 0
bool
operator==(IPAddr const &lhs, sockaddr const *rhs)
{
  bool zret = false;
  if (lhs._family == rhs->sa_family) {
    if (AF_INET == lhs._family) {
      zret = lhs._addr._ip4 == ats_ip4_addr_cast(rhs);
    } else if (AF_INET6 == lhs._family) {
      zret = 0 == memcmp(&lhs._addr._ip6, &ats_ip6_addr_cast(rhs), sizeof(in6_addr));
    } else { // map all non-IP to the same thing.
      zret = true;
    }
  } // else different families, not equal.
  return zret;
}

/** Compare two IP addresses.
    This is useful for IPv4, IPv6, and the unspecified address type.
    If the addresses are of different types they are ordered

    Non-IP < IPv4 < IPv6

     - all non-IP addresses are the same ( including @c AF_UNSPEC )
     - IPv4 addresses are compared numerically (host order)
     - IPv6 addresses are compared byte wise in network order (MSB to LSB)

    @return
      - -1 if @a lhs is less than @a rhs.
      - 0 if @a lhs is identical to @a rhs.
      - 1 if @a lhs is greater than @a rhs.
*/
int
IPAddr::cmp(self_type const &that) const
{
  int zret       = 0;
  uint16_t rtype = that._family;
  uint16_t ltype = _family;

  // We lump all non-IP addresses into a single equivalence class
  // that is less than an IP address. This includes AF_UNSPEC.
  if (AF_INET == ltype) {
    if (AF_INET == rtype) {
      in_addr_t la = ntohl(_addr._ip4);
      in_addr_t ra = ntohl(that._addr._ip4);
      if (la < ra) {
        zret = -1;
      } else if (la > ra) {
        zret = 1;
      } else {
        zret = 0;
      }
    } else if (AF_INET6 == rtype) { // IPv4 < IPv6
      zret = -1;
    } else { // IP > not IP
      zret = 1;
    }
  } else if (AF_INET6 == ltype) {
    if (AF_INET6 == rtype) {
      zret = memcmp(&_addr._ip6, &that._addr._ip6, TS_IP6_SIZE);
    } else {
      zret = 1; // IPv6 greater than any other type.
    }
  } else if (AF_INET == rtype || AF_INET6 == rtype) {
    // ltype is non-IP so it's less than either IP type.
    zret = -1;
  } else { // Both types are non-IP so they're equal.
    zret = 0;
  }

  return zret;
}

#endif

bool
IPAddr::is_multicast() const {
  return (AF_INET == _family && 0xe == (_addr._octet[0] >> 4)) ||
         (AF_INET6 == _family && IN6_IS_ADDR_MULTICAST(&_addr._ip6));
}

IPEndpoint::IPEndpoint(std::string_view const&text) {
  this->invalidate();
  this->parse(text);
}

bool IPMask::load(string_view const&text) {
  TextView parsed;
  _mask = swoc::svtou(text, &parsed);
  if (parsed.size() != text.size()) {
    _mask = 0;
    return false;
  }
  return true;
}

IP4Range::IP4Range(swoc::IP4Addr const&addr, swoc::IPMask const&mask) {
  this->assign(addr, mask);
}

IP4Range&IP4Range::assign(swoc::IP4Addr const&addr, swoc::IPMask const&mask) {
  // Special case the cidr sizes for 0, maximum
  if (0 == mask.width()) {
    _min = metric_type::MIN;
    _max = metric_type::MAX;
  } else {
    _min = _max = addr;
    if (mask.width() < 32) {
      in_addr_t bits = INADDR_BROADCAST << (32 - mask.width());
      _min._addr &= bits;
      _max._addr |= ~bits;
    }
  }
  return *this;
}

bool IP4Range::load(string_view text) {
  static const string_view SEPARATORS("/-");
  auto idx = text.find_first_of(SEPARATORS);
  if (idx != text.npos) {
    if (idx + 1 < text.size()) { // must have something past the separator or it's bogus.
      if ('/' == text[idx]) {
        metric_type addr;
        if (addr.load(text.substr(0, idx))) { // load the address
          IPMask mask;
          text.remove_prefix(idx + 1); // drop address and separator.
          if (mask.load(text)) {
            this->assign(addr, mask);
            return true;
          }
        }
      } else if (_min.load(text.substr(0, idx)) && _max.load(text.substr(idx + 1))) {
        return true;
      }
    }
  } else if (_min.load(text)) {
    _max = _min;
    return true;
  }

  this->clear();
  return false;
}

IP6Range & IP6Range::assign(IP6Addr const&addr, IPMask const&mask) {
  static constexpr auto FULL_MASK { std::numeric_limits<uint64_t>::max() };
  auto cidr = mask.width();
  if (cidr == 0) {
    _min = metric_type::MIN;
    _max = metric_type::MAX;
  } else if (cidr < 64) { // only upper bytes affected, lower bytes are forced.
    auto bits          = FULL_MASK << (64 - cidr);
    _min._addr._u64[0]           = addr._addr._u64[0] & bits;
    _min._addr._u64[1]           = 0;
    _max._addr._u64[0]           = addr._addr._u64[0] | ~bits;
    _max._addr._u64[1]           = FULL_MASK;
  } else if (cidr == 64) {
    _min._addr._u64[0] = _max._addr._u64[0] = addr._addr._u64[0];
    _min._addr._u64[1]                       = 0;
    _max._addr._u64[1]                       = FULL_MASK;
  } else if (cidr <= 128) { // _min bytes changed, _max bytes unaffected.
    _min = _max = addr;
    if (cidr < 128) {
      auto bits = FULL_MASK << (128 - cidr);
      _min._addr._u64[1] &= bits;
      _max._addr._u64[1] |= ~bits;
    }
  }
  return *this;
}

bool IP6Range::load(std::string_view text) {
  static const string_view SEPARATORS("/-");
  auto idx = text.find_first_of(SEPARATORS);
  if (idx != text.npos) {
    if (idx + 1 < text.size()) { // must have something past the separator or it's bogus.
      if ('/' == text[idx]) {
        metric_type addr;
        if (addr.load(text.substr(0, idx))) { // load the address
          IPMask mask;
          text.remove_prefix(idx + 1); // drop address and separator.
          if (mask.load(text)) {
            this->assign(addr, mask);
            return true;
          }
        }
      } else if (_min.load(text.substr(0, idx)) && _max.load(text.substr(idx + 1))) {
        return true;
      }
    }
  } else if (_min.load(text)) {
    _max = _min;
    return true;
  }
  this->clear();
  return false;
}

bool IPRange::load(std::string_view const& text) {
  static const string_view CHARS{".:"};
  auto idx = text.find_first_of(CHARS);
  if (idx != text.npos) {
    if (text[idx] == '.') {
      if (_range._ip4.load(text)) {
        _family = AF_INET;
        return true;
      }
    } else {
      if (_range._ip6.load(text)) {
        _family = AF_INET6;
        return true;
      }
    }
  }
  return false;
}

IPAddr IPRange::min() const {
  switch(_family) {
    case AF_INET: return _range._ip4.min();
    case AF_INET6: return _range._ip6.min();
    default: break;
  }
  return {};
}

IPAddr IPRange::max() const {
  switch(_family) {
    case AF_INET: return _range._ip4.max();
    case AF_INET6: return _range._ip6.max();
    default: break;
  }
  return {};
}

} // namespace swoc
