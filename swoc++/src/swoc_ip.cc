/** @file

  A brief file description

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
 */

#include <fstream>
#include "swoc/swoc_ip.h"
#include "swoc/swoc_meta.h"

using swoc::TextView;
using swoc::svtoi;
using swoc::svtou;

namespace
{
// sockaddr::sin_len - call this with a sockaddr type and Meta_Case_arg to set the sin_len
// member if it exists.
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

namespace swoc
{
IpAddr const IpAddr::INVALID;

bool
IpEndpoint::assign(sockaddr *dst, sockaddr const *src)
{
  size_t n = 0;
  if (dst != src) {
    self_type::invalidate(dst);
    switch (src->sa_family) {
    case AF_INET:
      n = sizeof(sockaddr_in);
      break;
    case AF_INET6:
      n = sizeof(sockaddr_in6);
      break;
    }
    if (n) {
      memcpy(dst, src, n);
    }
  }
  return n != 0;
}

IpEndpoint &
IpEndpoint::assign(IpAddr const &src, in_port_t port)
{
  switch (src.family()) {
  case AF_INET: {
    memset(&sa4, 0, sizeof sa4);
    sa4.sin_family      = AF_INET;
    sa4.sin_addr.s_addr = src.raw_ip4();
    sa4.sin_port        = port;
    Set_Sockaddr_Len(&sa4);
  } break;
  case AF_INET6: {
    memset(&sa6, 0, sizeof sa6);
    sa6.sin6_family = AF_INET6;
    sa6.sin6_addr   = src.raw_ip6();
    sa6.sin6_port   = port;
    Set_Sockaddr_Len(&sa6);
  } break;
  }
  return *this;
}

IpAddr &
IpAddr::assign(sockaddr const *addr)
{
  if (addr) {
    switch (addr->sa_family) {
    case AF_INET:
      return this->assign(reinterpret_cast<sockaddr_in const *>(addr)->sin_addr.s_addr);
    case AF_INET6:
      return this->assign(reinterpret_cast<sockaddr_in6 const *>(addr)->sin6_addr);
    default:
      break;
    }
  }
  _family = AF_UNSPEC;
  return *this;
}

sockaddr *
IpAddr::fill(sockaddr *sa, in_port_t port) const
{
  switch (sa->sa_family = _family) {
  case AF_INET: {
    sockaddr_in *sa4{reinterpret_cast<sockaddr_in *>(sa)};
    memset(sa4, 0, sizeof(*sa4));
    sa4->sin_addr.s_addr = _addr._ip4;
    sa4->sin_port        = port;
    Set_Sockaddr_Len(sa4);
  } break;
  case AF_INET6: {
    sockaddr_in6 *sa6{reinterpret_cast<sockaddr_in6 *>(sa)};
    memset(sa6, 0, sizeof(*sa6));
    sa6->sin6_port = port;
    Set_Sockaddr_Len(sa6);
  } break;
  }
  return sa;
}

std::string_view
IpEndpoint::family_name(uint16_t family)
{
  switch (family) {
  case AF_INET:
    return "ipv4"_sv;
  case AF_INET6:
    return "ipv6"_sv;
  case AF_UNIX:
    return "unix"_sv;
  case AF_UNSPEC:
    return "unspec"_sv;
  }
  return "unknown"_sv;
}

IpEndpoint &
IpEndpoint::set_to_any(int family)
{
  memset(*this, 0, sizeof(*this));
  if (AF_INET == family) {
    sa4.sin_family      = family;
    sa4.sin_addr.s_addr = INADDR_ANY;
    Set_Sockaddr_Len(&sa4);
  } else if (AF_INET6 == family) {
    sa6.sin6_family = family;
    sa6.sin6_addr   = in6addr_any;
    Set_Sockaddr_Len(&sa6);
  }
  return *this;
}

IpEndpoint &
IpEndpoint::set_to_loopback(int family)
{
  memset(*this, 0, sizeof(*this));
  if (AF_INET == family) {
    sa.sa_family        = family;
    sa4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Set_Sockaddr_Len(&sa4);
  } else if (AF_INET6 == family) {
    static const in6_addr init = IN6ADDR_LOOPBACK_INIT;
    sa.sa_family               = family;
    sa6.sin6_addr              = init;
    Set_Sockaddr_Len(&sa6);
  }
  return *this;
}

#if 0
bool
operator==(IpAddr const &lhs, sockaddr const *rhs)
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
IpAddr::cmp(self_type const &that) const
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
IpAddr::parse(const std::string_view &str)
{
  bool bracket_p  = false;
  uint16_t family = AF_UNSPEC;
  TextView src(str);
  _family = AF_UNSPEC; // invalidate until/unless success.
  src.trim_if(&isspace);
  if (*src == '[') {
    bracket_p = true;
    family    = AF_INET6;
    ++src;
  } else { // strip leading (hex) digits and see what the delimiter is.
    auto tmp = src;
    tmp.ltrim_if(&isxdigit);
    if (':' == *tmp) {
      family = AF_INET6;
    } else if ('.' == *tmp) {
      family = AF_INET;
    }
  }
  // Do the real parse now
  switch (family) {
  case AF_INET: {
    unsigned int n = 0; /// # of octets
    while (n < IP4_SIZE && !src.empty()) {
      TextView token{src.take_prefix_at('.')};
      TextView r;
      auto x = svtoi(token, &r, 10);
      if (r.size() == token.size()) {
        _addr._octet[n++] = x;
      } else {
        break;
      }
    }
    if (n == IP4_SIZE && src.empty()) {
      _family = AF_INET;
    }
  } break;
  case AF_INET6: {
    int n         = 0;
    int empty_idx = -1; // index of empty quad, -1 if not found yet.
    while (n < IP6_QUADS && !src.empty()) {
      TextView token{src.take_prefix_at(':')};
      if (token.empty()) {
        if (empty_idx > 0 || (empty_idx == 0 && n > 1)) {
          // two empty slots OK iff it's the first two (e.g. "::1"), otherwise invalid.
          break;
        }
        empty_idx = n;
      } else {
        TextView r;
        auto x = svtoi(token, &r, 16);
        if (r.size() == token.size()) {
          _addr._quad[n++] = htons(x);
        } else {
          break;
        }
      }
    }
    if (bracket_p) {
      src.ltrim_if(&isspace);
      if (']' != *src) {
        break;
      } else {
        ++src;
      }
    }
    // Handle empty quads - invalid if empty and still had a full set of quads
    if (empty_idx >= 0 && n < IP6_QUADS) {
      if (static_cast<int>(n) <= empty_idx) {
        while (empty_idx < static_cast<int>(IP6_QUADS)) {
          _addr._quad[empty_idx++] = 0;
        }
      } else {
        int k = 1;
        for (; n - k >= empty_idx; ++k) {
          _addr._quad[IP6_QUADS - k] = _addr._quad[n - k];
        }
        for (; IP6_QUADS - k >= empty_idx; ++k) {
          _addr._quad[IP6_QUADS - k] = 0;
          ++n; // track this so the validity check does the right thing.
        }
      }
    }
    if (n == IP6_QUADS && src.empty()) {
      _family = AF_INET6;
    }
  } break;
  }
  return this->is_valid();
}

bool
IpAddr::is_multicast() const
{
  return (AF_INET == _family && 0xe == (_addr._octet[0] >> 4)) || (AF_INET6 == _family && IN6_IS_ADDR_MULTICAST(&_addr._ip6));
}

IpEndpoint::IpEndpoint(std::string_view text)
{
  std::string_view addr, port;

  this->invalidate();
  if (this->tokenize(text, &addr, &port)) {
    IpAddr a(addr);
    if (a.is_valid()) {
      auto p = svtou(port);
      if (0 <= p && p < 65536) {
        this->assign(a, p);
      }
    }
  }
}

bool
IpEndpoint::tokenize(std::string_view str, std::string_view *addr, std::string_view *port, std::string_view *rest)
{
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
        *addr   = src.take_prefix_at(last);
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
IpRange::parse(std::string_view src)
{
  bool zret = false;
  static const IpAddr ZERO_ADDR4{INADDR_ANY};
  static const IpAddr MAX_ADDR4{INADDR_BROADCAST};
  static const IpAddr ZERO_ADDR6{in6addr_any};
  // I can't find a clean way to static const initialize an IPv6 address to all ones.
  // This is the best I can find that's portable.
  static const uint64_t ones[]{UINT64_MAX, UINT64_MAX};
  static const IpAddr MAX_ADDR6{reinterpret_cast<in6_addr const &>(ones)};

  auto idx = src.find_first_of("/-"_sv);
  if (idx != src.npos) {
    if (idx + 1 >= src.size()) { // must have something past the separator or it's bogus.
      ;                          // empty
    } else if ('/' == src[idx]) {
      if (_min.parse(src.substr(0, idx))) { // load the address
        TextView parsed;
        src.remove_prefix(idx + 1); // drop address and separator.
        int cidr = svtou(src, &parsed);
        if (parsed.size() && 0 <= cidr) { // a cidr that's a positive integer.
          // Special case the cidr sizes for 0, maximum, and for IPv6 64 bit boundaries.
          if (_min.is_ip4()) {
            zret = true;
            if (0 == cidr) {
              _min = ZERO_ADDR4;
              _max = MAX_ADDR4;
            } else if (cidr <= 32) {
              _max = _min;
              if (cidr < 32) {
                in_addr_t mask = htonl(INADDR_BROADCAST << (32 - cidr));
                _min._addr._ip4 &= mask;
                _max._addr._ip4 |= ~mask;
              }
            } else {
              zret = false;
            }
          } else if (_min.is_ip6()) {
            uint64_t mask;
            zret = true;
            if (cidr == 0) {
              _min = ZERO_ADDR6;
              _max = MAX_ADDR6;
            } else if (cidr < 64) { // only _max bytes affected, lower bytes are forced.
              mask         = htobe64(~static_cast<uint64_t>(0) << (64 - cidr));
              _max._family = _min._family;
              _min._addr._u64[0] &= mask;
              _min._addr._u64[1] = 0;
              _max._addr._u64[0] = _min._addr._u64[0] | ~mask;
              _max._addr._u64[1] = ~static_cast<uint64_t>(0);
            } else if (cidr == 64) {
              _max._family       = _min._family;
              _max._addr._u64[0] = _min._addr._u64[0];
              _min._addr._u64[1] = 0;
              _max._addr._u64[1] = ~static_cast<uint64_t>(0);
            } else if (cidr <= 128) { // lower bytes changed, _max bytes unaffected.
              _max = _min;
              if (cidr < 128) {
                mask = htobe64(~static_cast<uint64_t>(0) << (128 - cidr));
                _min._addr._u64[1] &= mask;
                _max._addr._u64[1] |= ~mask;
              }
            } else {
              zret = false;
            }
          }
        }
      }
    } else if (_min.parse(src.substr(0, idx)) && _max.parse(src.substr(idx + 1)) && _min.family() == _max.family()) {
      // not '/' so must be '-'
      zret = true;
    }
  } else if (_min.parse(src)) {
    zret = true;
    _max = _min;
  }

  if (!zret)
    this->clear();
  return zret;
}

BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, in_addr_t addr)
{
  auto *ptr = reinterpret_cast<uint8_t *>(&addr);
  bwf::Spec local_spec{spec}; // Format for address elements.
  bool align_p = false;

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      align_p          = true;
      local_spec._fill = '0';
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      align_p          = true;
      local_spec._fill = spec._ext[0];
    }
  }

  if (align_p) {
    local_spec._min   = 3;
    local_spec._align = bwf::Spec::Align::RIGHT;
  } else {
    local_spec._min = 0;
  }

  bwformat(w, local_spec, ptr[0]);
  w.write('.');
  bwformat(w, local_spec, ptr[1]);
  w.write('.');
  bwformat(w, local_spec, ptr[2]);
  w.write('.');
  bwformat(w, local_spec, ptr[3]);
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, in6_addr const &addr)
{
  using QUAD = uint16_t const;
  bwf::Spec local_spec{spec}; // Format for address elements.
  uint8_t const *ptr   = addr.s6_addr;
  uint8_t const *limit = ptr + sizeof(addr.s6_addr);
  QUAD *lower          = nullptr; // the best zero range
  QUAD *upper          = nullptr;
  bool align_p         = false;

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      align_p          = true;
      local_spec._fill = '0';
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      align_p          = true;
      local_spec._fill = spec._ext[0];
    }
  }

  if (align_p) {
    local_spec._min   = 4;
    local_spec._align = bwf::Spec::Align::RIGHT;
  } else {
    local_spec._min = 0;
    // do 0 compression if there's no internal fill.
    for (QUAD *spot = reinterpret_cast<QUAD *>(ptr), *last = reinterpret_cast<QUAD *>(limit), *current = nullptr; spot < last;
         ++spot) {
      if (0 == *spot) {
        if (current) {
          // If there's no best, or this is better, remember it.
          if (!lower || (upper - lower < spot - current)) {
            lower = current;
            upper = spot;
          }
        } else {
          current = spot;
        }
      } else {
        current = nullptr;
      }
    }
  }

  if (!local_spec.has_numeric_type()) {
    local_spec._type = 'x';
  }

  for (; ptr < limit; ptr += 2) {
    if (reinterpret_cast<uint8_t const *>(lower) <= ptr && ptr <= reinterpret_cast<uint8_t const *>(upper)) {
      if (ptr == addr.s6_addr) {
        w.write(':'); // only if this is the first quad.
      }
      if (ptr == reinterpret_cast<uint8_t const *>(upper)) {
        w.write(':');
      }
    } else {
      uint16_t f = (ptr[0] << 8) + ptr[1];
      bwformat(w, local_spec, f);
      if (ptr != limit - 2) {
        w.write(':');
      }
    }
  }
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, IpAddr const &addr)
{
  bwf::Spec local_spec{spec}; // Format for address elements and port.
  bool addr_p{true};
  bool family_p{false};

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      local_spec._ext.remove_prefix(1);
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      local_spec._ext.remove_prefix(2);
    }
  }
  if (local_spec._ext.size()) {
    addr_p = false;
    for (char c : local_spec._ext) {
      switch (c) {
      case 'a':
      case 'A':
        addr_p = true;
        break;
      case 'f':
      case 'F':
        family_p = true;
        break;
      }
    }
  }

  if (addr_p) {
    if (addr.is_ip4()) {
      bwformat(w, spec, addr.raw_ip4());
    } else if (addr.is_ip6()) {
      bwformat(w, spec, addr.raw_ip6());
    } else {
      w.print("*Not IP address [{}]*", addr.family());
    }
  }

  if (family_p) {
    local_spec._min = 0;
    if (addr_p) {
      w.write(' ');
    }
    if (spec.has_numeric_type()) {
      bwformat(w, local_spec, static_cast<uintmax_t>(addr.family()));
    } else {
      bwformat(w, local_spec, addr.family());
    }
  }
  return w;
}

BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, sockaddr const *addr)
{
  bwf::Spec local_spec{spec}; // Format for address elements and port.
  bool port_p{true};
  bool addr_p{true};
  bool family_p{false};
  bool local_numeric_fill_p{false};
  char local_numeric_fill_char{'0'};

  if (spec._type == 'p' || spec._type == 'P') {
    bwformat(w, spec, static_cast<void const *>(addr));
    return w;
  }

  if (spec._ext.size()) {
    if (spec._ext.front() == '=') {
      local_numeric_fill_p = true;
      local_spec._ext.remove_prefix(1);
    } else if (spec._ext.size() > 1 && spec._ext[1] == '=') {
      local_numeric_fill_p    = true;
      local_numeric_fill_char = spec._ext.front();
      local_spec._ext.remove_prefix(2);
    }
  }
  if (local_spec._ext.size()) {
    addr_p = port_p = false;
    for (char c : local_spec._ext) {
      switch (c) {
      case 'a':
      case 'A':
        addr_p = true;
        break;
      case 'p':
      case 'P':
        port_p = true;
        break;
      case 'f':
      case 'F':
        family_p = true;
        break;
      }
    }
  }

  if (addr_p) {
    bool bracket_p = false;
    switch (addr->sa_family) {
    case AF_INET:
      bwformat(w, spec, reinterpret_cast<sockaddr_in const *>(addr));
      break;
    case AF_INET6:
      if (port_p) {
        w.write('[');
        bracket_p = true; // take a note - put in the trailing bracket.
      }
      bwformat(w, spec, reinterpret_cast<sockaddr_in6 const *>(addr));
      break;
    default:
      w.print("*Not IP address [{}]*", addr->sa_family);
      break;
    }
    if (bracket_p)
      w.write(']');
    if (port_p)
      w.write(':');
  }
  if (port_p) {
    if (local_numeric_fill_p) {
      local_spec._min   = 5;
      local_spec._fill  = local_numeric_fill_char;
      local_spec._align = bwf::Spec::Align::RIGHT;
    } else {
      local_spec._min = 0;
    }
    bwformat(w, local_spec, static_cast<uintmax_t>(IpEndpoint::port(addr)));
  }
  if (family_p) {
    local_spec._min = 0;
    if (addr_p || port_p)
      w.write(' ');
    if (spec.has_numeric_type()) {
      bwformat(w, local_spec, static_cast<uintmax_t>(addr->sa_family));
    } else {
      bwformat(w, local_spec, IpEndpoint::family_name(addr->sa_family));
    }
  }
  return w;
}

} // namespace swoc
