#pragma once
// SPDX-License-Identifier: Apache-2.0
/** @file
   IP address and network related classes.
 */

#include <netinet/in.h>
#include <string_view>
#include <variant>

#include <swoc/DiscreteRange.h>
#include <swoc/RBTree.h>
#include "bwf_base.h"

namespace swoc
{
class IP4Addr;
class IP6Addr;
class IPAddr;
class IPMask;
class IP4Range;
class IP6Range;
class IPRange;

using ::std::string_view;

/** A union to hold @c sockaddr compliant IP address structures.

    This class contains a number of static methods to perform operations on external @c sockaddr
    instances. These are all duplicates of methods that operate on the internal @c sockaddr and
    are provided primarily for backwards compatibility during the shift to using this class.

    We use the term "endpoint" because these contain more than just the raw address, all of the data
    for an IP endpoint is present.
 */
union IPEndpoint {
  using self_type = IPEndpoint; ///< Self reference type.

  struct sockaddr sa;      ///< Generic address.
  struct sockaddr_in sa4;  ///< IPv4
  struct sockaddr_in6 sa6; ///< IPv6

  /// Default construct invalid instance.
  IPEndpoint();
  /// Construct from string representation of an address.
  IPEndpoint(string_view const &text);
  // Construct from @a IPAddr
  IPEndpoint(IPAddr const &addr);

  /** Break a string in to IP address relevant tokens.
   *
   * @param [in] src Source tex.t
   * @param [out] host The host / address.
   * @param [out] port The port.
   * @param [out] rest Any text past the end of the IP address.
   * @return @c true if an IP address was found, @c false otherwise.
   *
   * Any of the out parameters can be @c nullptr in which case they are not updated.
   * This parses and discards the IPv6 brackets.
   */
  static bool tokenize(string_view src, string_view *host = nullptr, string_view *port = nullptr, string_view *rest = nullptr);

  /** Parse a string for an IP address.

      The address resulting from the parse is copied to this object if the conversion is successful,
      otherwise this object is invalidated.

      @return @c true on success, @c false otherwise.
  */
  bool parse(string_view const &str);

  /// Invalidate a @c sockaddr.
  static void invalidate(sockaddr *addr);

  /// Invalidate this endpoint.
  self_type &invalidate();

  /// Copy constructor.
  self_type &operator=(self_type const &that);

  /** Copy (assign) the contents of @a src to @a dst.
   *
   * The caller must ensure @a dst is large enough to hold the contents of @a src, the size of which
   * can vary depending on the type of address in @a dst.
   *
   * @param dst Destination.
   * @param src Source.
   * @return @c true if @a dst is a valid IP address, @c false otherwise.
   */
  static bool assign(sockaddr *dst, sockaddr const *src);

  /** Assign from a socket address.
      The entire address (all parts) are copied if the @a ip is valid.
  */
  self_type &assign(sockaddr const *addr);

  /// Assign from an @a addr and @a port.
  self_type &assign(IPAddr const &addr, in_port_t port = 0);

  /// Copy to @a sa.
  const self_type &fill(sockaddr *addr) const;

  /// Test for valid IP address.
  bool is_valid() const;
  /// Test for IPv4.
  bool is_ip4() const;
  /// Test for IPv6.
  bool is_ip6() const;

  /** Effectively size of the address.
   *
   * @return The size of the structure appropriate for the address family of the stored address.
   */
  socklen_t size() const;

  /// @return The IP address family.
  sa_family_t family() const;

  /// Set to be the ANY address for family @a family.
  /// @a family must be @c AF_INET or @c AF_INET6.
  /// @return This object.
  self_type &set_to_any(int family);

  /// Set to be loopback for family @a family.
  /// @a family must be @c AF_INET or @c AF_INET6.
  /// @return This object.
  self_type &set_to_loopback(int family);

  /// Port in network order.
  in_port_t &port();
  /// Port in network order.
  in_port_t port() const;
  /// Port in host horder.
  in_port_t host_order_port() const;
  /// Port in network order from @a sockaddr.
  static in_port_t &port(sockaddr *sa);
  /// Port in network order from @a sockaddr.
  static in_port_t port(sockaddr const *sa);
  /// Port in host order directly from a @c sockaddr
  static in_port_t host_order_port(sockaddr const *sa);

  /// Automatic conversion to @c sockaddr.
  operator sockaddr *() { return &sa; }
  /// Automatic conversion to @c sockaddr.
  operator sockaddr const *() const { return &sa; }

  /// The string name of the address family.
  static string_view family_name(sa_family_t family);
};

/** Storage for an IPv4 address.
    Stored in network order.
 */
class IP4Addr {
  using self_type = IP4Addr; ///< Self reference type.
  friend class IP4Range;
public:
  static constexpr size_t SIZE = sizeof(in_addr_t); ///< Size of IPv4 address in bytes.
  static const self_type MIN;
  static const self_type MAX;

  constexpr IP4Addr() = default; ///< Default constructor - invalid result.

  /// Construct using IPv4 @a addr.
  explicit constexpr IP4Addr(in_addr_t addr);
  /// Construct from @c sockaddr_in.
  explicit IP4Addr(sockaddr_in const *addr);
  /// Construct from text representation.
  /// If the @a text is invalid the result is an invalid instance.
  IP4Addr(string_view const &text);

  /// Assign from IPv4 raw address.
  self_type &operator=(in_addr_t ip);
  /// Set to the address in @a addr.
  self_type &operator=(sockaddr_in const *addr);

  self_type &operator++();

  self_type &operator--();

  self_type &operator&=(IPMask const& mask);
  self_type &operator|=(IPMask const& mask);

  /// Write to @c sockaddr.
  sockaddr *fill(sockaddr_in *sa, in_port_t port = 0) const;

  in_addr_t network_order() const;

  /** Parse @a text as IPv4 address.
      The address resulting from the parse is copied to this object if the conversion is successful,
      otherwise this object is invalidated.

      @return @c true on success, @c false otherwise.
  */
  bool load(string_view const &text);

  /// Standard ternary compare.
  int cmp(self_type const &that) const;

  /// Get the IP address family.
  /// @return @c AF_INET
  sa_family_t family() const;

  /// Conversion to base type.
  operator in_addr_t() const;

  /// Test for multicast
  bool is_multicast() const;

  /// Test for loopback
  bool is_loopback() const;

protected:
  friend bool operator==(self_type const &, self_type const &);
  friend bool operator!=(self_type const &, self_type const &);
  friend bool operator<(self_type const &, self_type const &);
  friend bool operator<=(self_type const &, self_type const &);

  in_addr_t _addr = INADDR_ANY;
};

/** Storage for an IPv6 address.
    This should be presumed to be in network order.
 */
class IP6Addr {
  using self_type = IP6Addr; ///< Self reference type.
  friend class IP6Range;
public:
  using QUAD                      = uint16_t;            ///< Size of one segment of an IPv6 address.
  static constexpr size_t SIZE    = 16;    ///< Size of address in bytes.
  static constexpr size_t WORD_SIZE = sizeof(uint64_t); ///< Size of words used to store address.
  static constexpr size_t N_QUADS = SIZE / sizeof(QUAD); ///< # of quads in an IPv6 address.

  /// Type for the actual address.
  /// Equivalent to the data type for data member @c s6_addr in @c in6_addr.
  using raw_type = std::array<unsigned char, SIZE>;
  using quad_type = std::array<unsigned short, N_QUADS>;

  static const self_type MIN;
  static const self_type MAX;

  IP6Addr() = default; ///< Default constructor - 0 address.

  /// Construct using IPv6 @a addr.
  explicit IP6Addr(in6_addr const & addr);
  /// Construct from @c sockaddr_in.
  explicit IP6Addr(sockaddr_in6 const *addr) {
    *this = addr;
  }
  /// Construct from text representation.
  /// If the @a text is invalid the result is an invalid instance.
  IP6Addr(string_view const& text);

  self_type &operator++();

  self_type &operator--();

  /// Assign from IPv6 raw address.
  self_type &operator=(in6_addr const& ip);
  /// Set to the address in @a addr.
  self_type &operator=(sockaddr_in6 const *addr);

  /// Write to @c sockaddr using network order and @a port.
  sockaddr *copy_to(sockaddr *sa, in_port_t port = 0) const;

  /// Copy address to @a addr in network order.
  in6_addr & copy_to(in6_addr & addr) const;

  in6_addr network_order() const;

  /** Parse a string for an IP address.

      The address resuling from the parse is copied to this object if the conversion is successful,
      otherwise this object is invalidated.

      @return @c true on success, @c false otherwise.
  */
  bool load(string_view const &str);

  /// Generic compare.
  int cmp(self_type const &that) const;

  /// Get the address family.
  /// @return The address family.
  sa_family_t family() const;

  /// Test for multicast
  bool is_multicast() const;

  /// Test for loopback
  bool is_loopback() const;

  self_type & clear() {
    _addr._u64[0] = _addr._u64[1] = 0;
    return *this;
  }

  static void reorder(in6_addr & dst, raw_type const & src);
  static void reorder(raw_type & dst, in6_addr const& src);
  static void reorder(unsigned char dst[WORD_SIZE], unsigned char const src[WORD_SIZE]);

protected:
  friend bool operator==(self_type const &, self_type const &);
  friend bool operator!=(self_type const &, self_type const &);
  friend bool operator<(self_type const &, self_type const &);
  friend bool operator<=(self_type const &, self_type const &);
  union {
    uint64_t _u64[2] = {0, 0}; ///< 0 is MSW, 1 is LSW.
    quad_type _quad; ///< By quad.
    raw_type _raw; ///< By byte.
  } _addr;

  static constexpr std::array<unsigned, N_QUADS> QUAD_IDX = { 3,2,1,0,7,6,5,4 };

  IP6Addr(uint64_t msw, uint64_t lsw) : _addr{msw, lsw} {}
};

/** Storage for an IP address.
 */
class IPAddr {
  friend class IPRange;
  using self_type = IPAddr; ///< Self reference type.
public:
  IPAddr() = default; ///< Default constructor - invalid result.
  IPAddr(self_type const& that) = default; ///< Copy constructor.

  /// Construct using IPv4 @a addr.
  explicit IPAddr(in_addr_t addr);
  /// Construct using an IPv4 @a addr
  IPAddr(IP4Addr const &addr) : _family(AF_INET), _addr{addr} {}
  /// Construct using IPv6 @a addr.
  explicit IPAddr(in6_addr const &addr);
  /// construct using an IPv6 @a addr
  IPAddr(IP6Addr const &addr) : _family(AF_INET6), _addr{addr} {}
  /// Construct from @c sockaddr.
  explicit IPAddr(sockaddr const *addr);
  /// Construct from @c IPEndpoint.
  explicit IPAddr(IPEndpoint const &addr);
  /// Construct from text representation.
  /// If the @a text is invalid the result is an invalid instance.
  explicit IPAddr(string_view text);

  /// Set to the address in @a addr.
  self_type &assign(sockaddr const *addr);
  /// Set to the address in @a addr.
  self_type &assign(sockaddr_in const *addr);
  /// Set to the address in @a addr.
  self_type &assign(sockaddr_in6 const *addr);
  /// Set to the address in @a addr.
  self_type &assign(in_addr_t addr);
  /// Set to address in @a addr.
  self_type &assign(in6_addr const &addr);

  /// Assign from end point.
  self_type &operator=(IPEndpoint const &ip);
  /// Assign from IPv4 raw address.
  self_type &operator=(in_addr_t ip);
  /// Assign from IPv6 raw address.
  self_type &operator=(in6_addr const &ip);
  /// Assign from @c sockaddr
  self_type &operator=(sockaddr const *addr);

  self_type &operator&=(IPMask const& mask);
  self_type &operator|=(IPMask const& mask);

  /// Write to @c sockaddr.
  sockaddr *fill(sockaddr *sa, in_port_t port = 0) const;

  /** Parse a string and load the result in @a this.
   *
   * @param text Text to parse.
   * @return  @c true on success, @c false otherwise.
   */
  bool load(string_view const &text);

  /// Generic compare.
  int cmp(self_type const &that) const;

  /// Test for same address family.
  /// @c return @c true if @a that is the same address family as @a this.
  bool isCompatibleWith(self_type const &that);

  /// Get the address family.
  /// @return The address family.
  sa_family_t family() const;
  /// Test for IPv4.
  bool is_ip4() const;
  /// Test for IPv6.
  bool is_ip6() const;

  in_addr_t network_ip4() const;
  in6_addr network_ip6() const;

  /// Test for validity.
  bool is_valid() const;

  /// Make invalid.
  self_type &invalidate();

  /// Test for multicast
  bool is_multicast() const;

  /// Test for loopback
  bool is_loopback() const;

  ///< Pre-constructed invalid instance.
  static self_type const INVALID;

protected:
  friend bool operator==(self_type const &, self_type const &);

  /// Address data.
  union raw_addr_type {
    IP4Addr _ip4; ///< IPv4 address (host)
    IP6Addr _ip6;                                    ///< IPv6 address (host)
    uint8_t _octet[IP6Addr::SIZE];                   ///< IPv4 octets.
    uint64_t _u64[IP6Addr::SIZE / sizeof(uint64_t)]; ///< As 64 bit chunks.

    constexpr raw_addr_type();
    raw_addr_type(in_addr_t addr) : _ip4(addr) {}
    raw_addr_type(in6_addr const& addr) : _ip6(addr) {}

    raw_addr_type(IP4Addr const& addr) : _ip4(addr) {}
    raw_addr_type(IP6Addr const& addr) : _ip6(addr) {}
  } _addr;

  sa_family_t _family{AF_UNSPEC}; ///< Protocol family.
};

/** An IP address mask.
 *
 * This is essentially a width for a bit mask.
 */
class IPMask {
  using self_type = IPMask;  ///< Self reference type.
  using raw_type  = uint8_t; ///< Storage for mask width.

public:
  IPMask() = default;
  IPMask(raw_type count, sa_family_t family = AF_INET);
  IPMask(string_view text);

  bool load(string_view const& text);

  /** Get the CIDR mask wide enough to cover this address.
   * @param addr Input address.
   * @return Effectively the reverse index of the least significant bit set to 1.
   */
  int cidr_of(IPAddr addr);

  /// The width of the mask.
  raw_type width() const;

  /// Family type.
  sa_family_t family() const;

  /// Write the mask as an address to @a addr.
  /// @return The filled address.
  IPAddr &fill(IPAddr &addr);

private:
  raw_type _mask{0};
  sa_family_t _family{AF_UNSPEC};
};


/** An inclusive range of IPv4 addresses.
 */
class IP4Range : public DiscreteRange<IP4Addr> {
  using self_type  = IP4Range;
  using super_type = DiscreteRange<IP4Addr>;
  using metric_type = IP4Addr;

public:
  using super_type::super_type; ///< Import super class constructors.

  IP4Range() = default;
  IP4Range(IP4Addr const& addr, IPMask const& mask);

  /** Construct range from @a text.
   *
   * @param text Range text.
   * @see IP4Range::load
   *
   * This results in a zero address if @a text is not a valid string. If this should be checked,
   * use @c load.
   */
  IP4Range(string_view text) {
    this->load(text);
  }

  self_type & assign(IP4Addr const& addr, IPMask const& mask);

  /** Assign to this range from text.
   * The text must be in one of three formats.
   * - A dashed range, "addr1-addr2"
   * - A singleton, "addr". This is treated as if it were "addr-addr", a range of size 1.
   * - CIDR notation, "addr/cidr" where "cidr" is a number from 0 to the number of bits in the address.
   * @param text Range text.
   */
  bool load(string_view text);

protected:
};

class IP6Range : public DiscreteRange<IP6Addr> {
  using self_type  = IP6Range;
  using super_type = DiscreteRange<IP6Addr>;

public:
  using super_type::super_type; ///< Import super class constructors.

  self_type & assign(IP6Addr const& addr, IPMask const& mask);

  bool load(string_view text);

};

class IPRange {
  using self_type = IPRange;
public:
  IPRange() = default;

  bool is(sa_family_t f) const {
    return f == _family;
  }

  /** Load the range from @a text.
   *
   * @param text Range specifier in text format.
   * @return @c true if @a text was successfully parsed, @c false if not.
   *
   * A successful parse means @a this was loaded with the specified range. If not the range is
   * marked as invalid.
   */
  bool load(std::string_view const& text);

  IPAddr min() const;
  IPAddr max() const;

  operator IP4Range & () { return _range._ip4; }
  operator IP6Range & () { return _range._ip6; }
  operator IP4Range const & () const { return _range._ip4; }
  operator IP6Range const & () const { return _range._ip6; }

protected:
  /** Range container.
   *
   * @internal
   *
   * This was a @c std::variant at one point, but the complexity got in the way because
   * - These objects have no state, need no destruction.
   * - Construction was problematic because @c variant requires construction, then access,
   *   whereas this needs access to construct (e.g. via the @c load method).
   */
  union {
    std::monostate _nil; ///< Make constructor easier to implement.
    IP4Range _ip4; ///< IPv4 range.
    IP6Range _ip6; ///< IPv6 range.
  } _range { std::monostate{} };
  /// Family of @a _range.
  sa_family_t _family { AF_UNSPEC };
};

/** Representation of an IP address network.
 *
 */
class IpNet {
  using self_type = IpNet; ///< Self reference type.
public:
  IpNet();
  IpNet(const IPAddr &addr, const IPMask &mask);

  operator IPAddr const &() const;
  operator IPMask const &() const;

  IPAddr const &addr() const;

  IPMask const &mask() const;

  IPAddr lower_bound() const;
  IPAddr upper_bound() const;

  bool contains(IPAddr const &addr) const;

  // computes this is strict subset of other
  bool is_subnet_of(self_type const &that);

  // Check if there are any addresses in both @a this and @a that.
  bool intersects(self_type const &that);

  self_type &assign(IPAddr const &addr, IPMask const &mask);

  static char const SEPARATOR; // the character used between the address and mask

  operator ::std::string() const; // implicit
  ::std::string ntoa() const;     // explicit
  // the address width is per octet, the mask width for the bit count
  ::std::string ntoa(int addr_width, int mask_width) const;

  static ::std::string ntoa(IpNet const &net); // DEPRECATED
  static IpNet aton(::std::string const &str); // DEPRECATED - use ctor

protected:
  IPAddr _addr;
  IPMask _mask;
};

// --------------------------------------------------------------------------
/** Coloring of IP address space.
 *
 * @tparam PAYLOAD The color class.
 *
 * This is a class to do fast coloring and lookup of the IP address space. It is range oriented and
 * performs well for ranges, much less well for singletons. Conceptually every IP address is a key
 * in the space and can have a color / payload of type @c PAYLOAD.
 *
 * @c PAYLOAD must have the properties
 *
 * - Cheap to copy.
 * - Comparable via the equality and inequality operators.
 */
template <typename PAYLOAD> class IPSpace {
  using self_type = IPSpace;
  using IP4Space  = DiscreteSpace<IP4Addr, PAYLOAD>;
  using IP6Space  = DiscreteSpace<IP6Addr, PAYLOAD>;

public:
  using payload_t = PAYLOAD; ///< Export payload type.

  /// Construct an empty space.
  IPSpace() = default;

  /** Mark the range @a r with @a payload.
   *
   * @param r Range to mark.
   * @param payload Payload to assign.
   * @return @a this
   *
   * All addresses in @a r are set to have the @a payload.
   */
  self_type & mark(IP4Range const &r, PAYLOAD const &payload);

  /** Fill the @a range with @a payload.
   *
   * @param range Destination range.
   * @param payload Payload for range.
   * @return this
   *
   * Addresses in @a range are set to have @a payload if the address does not already have a payload.
   */
  self_type & fill(IPRange const& range, PAYLOAD const& payload);

  /** Blend @a payload in to the @a range.
   *
   * @tparam F Blending functor type (deduced).
   * @param range Target range.
   * @param payload Payload for @a range.
   * @param blender Blending functor.
   * @return @a this
   *
   * @a blender is required to have the signature <tt>void(PAYLOAD& lhs , PAYLOAD CONST&rhs)</tt>.
   * It must act as a compound assignment operator, blending @a B into @a A. That is, if the
   * result of blending B in to A is defined as "A @ B" for the binary operator "@", then
   * @a blender computes "A @= B".
   *
   * Every address in @a range is assigned a payload. If the address does not already have a
   * payload, it is assigned @a payload. If the address has a payload of A it is updated by
   * invoking @a blender on the existing payload and @a payload.
   */
  template < typename F >
  self_type & blend(IPRange const& range, PAYLOAD const& payload, F && blender);

  template < typename F >
  self_type & blend(IP4Range const& range, PAYLOAD const& payload, F && blender) {
    _ip4.blend(range, payload, blender);
    return *this;
  }

  template < typename F >
  self_type & blend(IP6Range const& range, PAYLOAD const& payload, F && blender) {
    _ip6.blend(range, payload, blender);
    return *this;
  }

  /** Find the payload for an @a addr.
   *
   * @param addr Address to find.
   * @return The payload if any, @c nullptr if the address is not in the space.
   */
  PAYLOAD *find(IP4Addr const &addr) {
    return _ip4.find(addr);
  }

  /// @return The number of distinct ranges.
  size_t count() const { return _ip4.count() + _ip6.count(); }

  /// Remove all ranges.
  void clear();

  typename IP4Space::iterator begin() { return _ip4.begin(); }
  typename IP4Space::iterator end() { return _ip4.end(); }

protected:
  IP4Space _ip4;
  IP6Space _ip6;
};
// --------------------------------------------------------------------------

// @c constexpr constructor is required to initialize _something_, it can't be completely uninitializing.
inline constexpr IPAddr::raw_addr_type::raw_addr_type() : _ip4(INADDR_ANY) {}

inline IPAddr::IPAddr(in_addr_t addr) : _family(AF_INET), _addr(addr) {}

inline IPAddr::IPAddr(in6_addr const &addr) : _family(AF_INET6), _addr(addr) {}

inline IPAddr::IPAddr(sockaddr const *addr) {
  this->assign(addr);
}

inline IPAddr::IPAddr(IPEndpoint const &addr) {
  this->assign(&addr.sa);
}

inline IPAddr &
IPAddr::operator=(in_addr_t addr) {
  _family    = AF_INET;
  _addr._ip4 = addr;
  return *this;
}

inline IPAddr &
IPAddr::operator=(in6_addr const &addr) {
  _family    = AF_INET6;
  _addr._ip6 = addr;
  return *this;
}

inline IPAddr &
IPAddr::operator=(IPEndpoint const &addr) {
  return this->assign(&addr.sa);
}

inline IPAddr &
IPAddr::operator=(sockaddr const *addr) {
  return this->assign(addr);
}

inline sa_family_t
IPAddr::family() const {
  return _family;
}

inline bool
IPAddr::is_ip4() const {
  return AF_INET == _family;
}

inline bool
IPAddr::is_ip6() const {
  return AF_INET6 == _family;
}

inline bool
IPAddr::isCompatibleWith(self_type const &that) {
  return this->is_valid() && _family == that._family;
}

inline bool
IPAddr::is_loopback() const {
  return (AF_INET == _family && 0x7F == _addr._octet[0]) || (AF_INET6 == _family && IN6_IS_ADDR_LOOPBACK(&_addr._ip6));
}

inline bool
operator==(IPAddr const &lhs, IPAddr const &rhs) {
  if (lhs._family != rhs._family)
    return false;
  switch (lhs._family)
  {
  case AF_INET:
    return lhs._addr._ip4 == rhs._addr._ip4;
  case AF_INET6:
    return 0 == memcmp(&lhs._addr._ip6, &rhs._addr._ip6, IP6Addr::SIZE);
  case AF_UNSPEC:
    return true;
  default:
    break;
  }
  return false;
}

inline bool
operator!=(IPAddr const &lhs, IPAddr const &rhs) {
  return !(lhs == rhs);
}

inline IPAddr &
IPAddr::assign(in_addr_t addr) {
  _family    = AF_INET;
  _addr._ip4 = addr;
  return *this;
}

inline IPAddr &
IPAddr::assign(in6_addr const &addr) {
  _family    = AF_INET6;
  _addr._ip6 = addr;
  return *this;
}

inline IPAddr &
IPAddr::assign(sockaddr_in const *addr) {
  if (addr)
  {
    _family    = AF_INET;
    _addr._ip4 = addr->sin_addr.s_addr;
  } else
  { _family = AF_UNSPEC; }
  return *this;
}

inline IPAddr &
IPAddr::assign(sockaddr_in6 const *addr) {
  if (addr)
  {
    _family    = AF_INET6;
    _addr._ip6 = addr->sin6_addr;
  } else
  { _family = AF_UNSPEC; }
  return *this;
}

inline bool
IPAddr::is_valid() const {
  return _family == AF_INET || _family == AF_INET6;
}

inline IPAddr &
IPAddr::invalidate() {
  _family = AF_UNSPEC;
  return *this;
}

// Associated operators.
bool operator==(IPAddr const &lhs, sockaddr const *rhs);
inline bool
operator==(sockaddr const *lhs, IPAddr const &rhs) {
  return rhs == lhs;
}
inline bool
operator!=(IPAddr const &lhs, sockaddr const *rhs) {
  return !(lhs == rhs);
}
inline bool
operator!=(sockaddr const *lhs, IPAddr const &rhs) {
  return !(rhs == lhs);
}
inline bool
operator==(IPAddr const &lhs, IPEndpoint const &rhs) {
  return lhs == &rhs.sa;
}
inline bool
operator==(IPEndpoint const &lhs, IPAddr const &rhs) {
  return &lhs.sa == rhs;
}
inline bool
operator!=(IPAddr const &lhs, IPEndpoint const &rhs) {
  return !(lhs == &rhs.sa);
}
inline bool
operator!=(IPEndpoint const &lhs, IPAddr const &rhs) {
  return !(rhs == &lhs.sa);
}

inline bool
operator<(IPAddr const &lhs, IPAddr const &rhs) {
  return -1 == lhs.cmp(rhs);
}

inline bool
operator>=(IPAddr const &lhs, IPAddr const &rhs) {
  return lhs.cmp(rhs) >= 0;
}

inline bool
operator>(IPAddr const &lhs, IPAddr const &rhs) {
  return 1 == lhs.cmp(rhs);
}

inline bool
operator<=(IPAddr const &lhs, IPAddr const &rhs) {
  return lhs.cmp(rhs) <= 0;
}

inline in_addr_t
IPAddr::network_ip4() const {
  return _addr._ip4.network_order();
}

inline in6_addr
IPAddr::network_ip6() const {
  return _addr._ip6.network_order();
}

/// ------------------------------------------------------------------------------------

inline IPEndpoint::IPEndpoint() {
  sa.sa_family = AF_UNSPEC;
}

inline IPEndpoint::IPEndpoint(IPAddr const &addr) {
  this->assign(addr);
}

inline IPEndpoint &
IPEndpoint::invalidate() {
  sa.sa_family = AF_UNSPEC;
  return *this;
}

inline void
IPEndpoint::invalidate(sockaddr *addr) {
  addr->sa_family = AF_UNSPEC;
}

inline bool
IPEndpoint::is_valid() const {
  return sa.sa_family == AF_INET || sa.sa_family == AF_INET6;
}

inline IPEndpoint &
IPEndpoint::operator=(self_type const &that) {
  self_type::assign(&sa, &that.sa);
  return *this;
}

inline IPEndpoint &
IPEndpoint::assign(sockaddr const *src) {
  self_type::assign(&sa, src);
  return *this;
}

inline IPEndpoint const &
IPEndpoint::fill(sockaddr *addr) const {
  self_type::assign(addr, &sa);
  return *this;
}

inline bool
IPEndpoint::is_ip4() const {
  return AF_INET == sa.sa_family;
}

inline bool
IPEndpoint::is_ip6() const {
  return AF_INET6 == sa.sa_family;
}

inline sa_family_t
IPEndpoint::family() const {
  return sa.sa_family;
}

inline in_port_t &
IPEndpoint::port() {
  return self_type::port(&sa);
}

inline in_port_t
IPEndpoint::port() const {
  return self_type::port(&sa);
}

inline in_port_t
IPEndpoint::host_order_port() const {
  return ntohs(this->port());
}

inline in_port_t &
IPEndpoint::port(sockaddr *sa) {
  switch (sa->sa_family)
  {
  case AF_INET:
    return reinterpret_cast<sockaddr_in *>(sa)->sin_port;
  case AF_INET6:
    return reinterpret_cast<sockaddr_in6 *>(sa)->sin6_port;
  }
  // Force a failure upstream by returning a null reference.
  return *static_cast<in_port_t *>(nullptr);
}

inline in_port_t
IPEndpoint::port(sockaddr const *addr) {
  return self_type::port(const_cast<sockaddr *>(addr));
}

inline in_port_t
IPEndpoint::host_order_port(sockaddr const *addr) {
  return ntohs(self_type::port(addr));
}

// --- IPAddr variants ---

inline constexpr IP4Addr::IP4Addr(in_addr_t addr) : _addr(addr) {}

inline IP4Addr::IP4Addr(std::string_view const& text) {
  if (! this->load(text)) {
    _addr = INADDR_ANY;
  }
}

inline IP4Addr &
IP4Addr::operator++() {
  ++_addr;
  return *this;
}

inline IP4Addr &
IP4Addr::operator--() {
  --_addr;
  return *this;
}

inline in_addr_t IP4Addr::network_order() const {
  return htonl(_addr);
}

inline IP4Addr::operator in_addr_t() const {
  return this->network_order();
}

inline auto
IP4Addr::operator=(in_addr_t ip) -> self_type & {
  _addr = ntohl(ip);
  return *this;
}

inline bool operator == (IP4Addr const& lhs, IP4Addr const& rhs) {
  return lhs._addr == rhs._addr;
}

inline bool operator != (IP4Addr const& lhs, IP4Addr const& rhs) {
  return lhs._addr != rhs._addr;
}

inline bool operator < (IP4Addr const& lhs, IP4Addr const& rhs) {
  return lhs._addr < rhs._addr;
}

inline bool operator <= (IP4Addr const& lhs, IP4Addr const& rhs) {
  return lhs._addr <= rhs._addr;
}

inline bool operator > (IP4Addr const& lhs, IP4Addr const& rhs) {
  return rhs < lhs;
}

inline bool operator >= (IP4Addr const& lhs, IP4Addr const& rhs) {
  return rhs <= lhs;
}

inline IP4Addr & IP4Addr::operator&=(IPMask const& mask) {
  auto n = std::min<unsigned>(32, mask.width());
  in_addr_t bits = htonl(INADDR_BROADCAST << (32 - n));
  _addr &= bits;
  return *this;
}

inline IP4Addr & IP4Addr::operator|=(IPMask const& mask) {
  auto n = std::min<unsigned>(32, mask.width());
  in_addr_t bits = htonl(INADDR_BROADCAST << (32 - n));
  _addr |= bits;
  return *this;
}

// ---

inline IP6Addr::IP6Addr(in6_addr const& addr) {
  *this = addr;
}

inline IP6Addr::IP6Addr(std::string_view const& text) {
  if (! this->load(text)) {
    this->clear();
  }
}

inline in6_addr& IP6Addr::copy_to(in6_addr & addr) const {
  self_type::reorder(addr, _addr._raw);
  return addr;
}

inline in6_addr IP6Addr::network_order() const {
  in6_addr zret;
  return this->copy_to(zret);
}

inline auto IP6Addr::operator = (in6_addr const& addr) -> self_type & {
  self_type::reorder(_addr._raw, addr);
  return *this;
}

inline auto IP6Addr::operator = (sockaddr_in6 const* addr) -> self_type & {
  if (addr) {
    return *this = addr->sin6_addr;
  }
  this->clear();
}

inline IP6Addr &
IP6Addr::operator++() {
  if (++(_addr._u64[1]) == 0) {
    ++(_addr._u64[0]);
  }
  return *this;
}

inline IP6Addr &
IP6Addr::operator--() {
  if (--(_addr._u64[1]) == ~static_cast<uint64_t >(0)) {
    --(_addr._u64[0]);
  }
  return *this;
}

inline void IP6Addr::reorder(unsigned char dst[WORD_SIZE], unsigned char const src[WORD_SIZE]) {
  for ( size_t idx = 0 ; idx < WORD_SIZE ; ++idx ) {
    dst[idx] = src[WORD_SIZE - (idx + 1)];
  }
}

inline bool operator == (IP6Addr const& lhs, IP6Addr const& rhs) {
  return lhs._addr._u64[0] == rhs._addr._u64[0] &&
         lhs._addr._u64[1] == rhs._addr._u64[1];
}

inline bool operator != (IP6Addr const& lhs, IP6Addr const& rhs) {
  return lhs._addr._u64[0] != rhs._addr._u64[0] ||
         lhs._addr._u64[1] != rhs._addr._u64[1];
}

inline bool operator < (IP6Addr const& lhs, IP6Addr const& rhs) {
  return lhs._addr._u64[0] < rhs._addr._u64[0] || (lhs._addr._u64[0] == rhs._addr._u64[0] && lhs._addr._u64[1] < rhs._addr._u64[1]);
}

inline bool operator > (IP6Addr const& lhs, IP6Addr const& rhs) {
  return rhs < lhs;
}

inline bool operator <= (IP6Addr const& lhs, IP6Addr const& rhs) {
  return lhs._addr._u64[0] < rhs._addr._u64[0] || (lhs._addr._u64[0] == rhs._addr._u64[0] && lhs._addr._u64[1] <= rhs._addr._u64[1]);
}

inline bool operator >= (IP6Addr const& lhs, IP6Addr const& rhs) {
  return rhs <= lhs;
}

// +++ IPRange +++

// +++ IpMask +++

inline IPMask::IPMask(raw_type width, sa_family_t family) : _mask(width), _family(family) {}

inline IPMask::raw_type
IPMask::width() const {
  return _mask;
}

inline bool
operator==(IPMask const &lhs, IPMask const &rhs) {
  return lhs.width() == rhs.width();
}

inline bool
operator!=(IPMask const &lhs, IPMask const &rhs) {
  return lhs.width() != rhs.width();
}

inline bool
operator<(IPMask const &lhs, IPMask const &rhs) {
  return lhs.width() < rhs.width();
}

inline IpNet::IpNet() {}

inline IpNet::IpNet(IPAddr const &addr, IPMask const &mask) : _addr(addr), _mask(mask) {}

inline IpNet::operator IPAddr const &() const {
  return _addr;
}

inline IpNet::operator IPMask const &() const {
  return _mask;
}

inline IPAddr const &
IpNet::addr() const {
  return _addr;
}

inline IPMask const &
IpNet::mask() const {
  return _mask;
}

// --- IPSpace

template < typename PAYLOAD > auto IPSpace<PAYLOAD>::mark(swoc::IP4Range const &r, PAYLOAD const &payload) -> self_type & {
  _ip4.mark(r, payload);
  return *this;
}

template < typename PAYLOAD > auto IPSpace<PAYLOAD>::fill(swoc::IPRange const &range, PAYLOAD const &payload) -> self_type & {
  if (range.is(AF_INET6)) {
    _ip6.fill(range, payload);
  } else if (range.is(AF_INET)) {
    _ip4.fill(range, payload);
  }
  return *this;
}

template<typename PAYLOAD>
template<typename F>
auto IPSpace<PAYLOAD>::blend(IPRange const&range, PAYLOAD const&payload, F&&blender) -> self_type &{
  if (range.is(AF_INET)) {
    _ip4.blend(range, payload, blender);
  } else if (range.is(AF_INET6)) {
    _ip6.blend(range, payload, blender);
  }
  return *this;
}

template<typename PAYLOAD>
void IPSpace<PAYLOAD>::clear() {
  _ip4.clear();
  _ip6.clear();
}


} // namespace swoc
