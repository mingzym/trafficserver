/** @file

  IP related functions.

  This includes various casting functions for working with the
  @c sockaddr_storage data type. For generality this type is used
  for storing IP address related information. In most cases this
  is sufficient as the data can be used directly with system calls.
  The casting functionsa are used when more specific information is
  needed from the data.

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

#if !defined (_ink_inet_h_)
#define _ink_inet_h_

#include "ink_platform.h"
#include "ink_port.h"
#include "ink_apidefs.h"
# include <ts/ink_assert.h>

#define INK_GETHOSTBYNAME_R_DATA_SIZE 1024
#define INK_GETHOSTBYADDR_R_DATA_SIZE 1024

/// Size in bytes of an IPv6 address.
size_t const INK_IP6_SIZE = 16;

struct ink_gethostbyname_r_data {
  int herrno;
  struct hostent ent;
  char buf[INK_GETHOSTBYNAME_R_DATA_SIZE];
};

struct ink_gethostbyaddr_r_data
{
  int herrno;
  struct hostent ent;
  char buf[INK_GETHOSTBYADDR_R_DATA_SIZE];
};

/**
  returns the IP address of the hostname. If the hostname has
  multiple IP addresses, the first IP address in the list returned
  by 'gethostbyname' is returned.

  @note Not thread-safe

*/
unsigned int host_to_ip(char *hostname);

/**
  Wrapper for gethostbyname_r(). If successful, returns a pointer
  to the hostent structure. Returns NULL and sets data->herrno to
  the appropriate error code on failure.

  @param hostname null-terminated host name string
  @param data pointer to ink_gethostbyname_r_data allocated by the caller

*/
struct hostent *ink_gethostbyname_r(char *hostname, ink_gethostbyname_r_data * data);

/**
  Wrapper for gethostbyaddr_r(). If successful, returns a pointer
  to the hostent structure. Returns NULL and sets data->herrno to
  the appropriate error code on failure.

  @param ip IP address of the host
  @param len length of the buffer indicated by ip
  @param type family of the address
  @param data pointer to ink_gethostbyname_r_data allocated by the caller

*/
struct hostent *ink_gethostbyaddr_r(char *ip, int len, int type, ink_gethostbyaddr_r_data * data);

/**
  Wrapper for inet_addr().

  @param s IP address in the Internet standard dot notation.

*/
inkcoreapi uint32_t ink_inet_addr(const char *s);

/** Write a null terminated string for @a addr to @a dst.
    A buffer of size INET6_ADDRSTRLEN suffices, including a terminating nul.
 */
char const* ink_inet_ntop(
  const sockaddr_storage *addr, ///< Address.
  char *dst, ///< Output buffer.
  size_t size ///< Length of buffer.
);

static size_t const INET6_ADDRPORTSTRLEN = INET6_ADDRSTRLEN + 6;

/** Write a null terminated string for @a addr to @a dst with port.
    A buffer of size INET6_ADDRPORTSTRLEN suffices, including a terminating nul.
 */
char const* ink_inet_nptop(
  const sockaddr_storage *addr, ///< Address.
  char *dst, ///< Output buffer.
  size_t size ///< Length of buffer.
);

/** Convert @a text to an IP address and write it to @a addr.

    @a test is expected to be an explicit address, not a hostname.  No
    hostname resolution is done.

    @note This uses @c getaddrinfo internally and so involves memory
    allocation.

    @return 0 on success, non-zero on failure.
*/
int ink_inet_pton(
  char const* text, ///< [in] text.
  sockaddr_storage* addr ///< [out] address
);

/// Reset an address to invalid.
/// @note Useful for marking a member as not yet set.
inline void ink_inet_invalidate(sockaddr_storage* addr) {
  addr->ss_family = AF_UNSPEC;
}
/// Reset an address to invalid.
/// Convenience overload.
/// @note Useful for marking a member as not yet set.
inline void ink_inet_invalidate(sockaddr_storage& addr) {
  addr.ss_family = AF_UNSPEC;
}
/// Test for validity.
/// @return @c false if the address is reset, @c true otherwise.
inline bool ink_inet_is_valid(sockaddr_storage const* addr) {
  return addr->ss_family != AF_UNSPEC;
}
/// Test for validity.
/// Convenience overload.
/// @return @c false if the address is reset, @c true otherwise.
inline bool ink_inet_is_valid(sockaddr_storage const& addr) {
  return addr.ss_family != AF_UNSPEC;
}
/// Set to all zero.
inline void ink_inet_init(sockaddr_storage& addr) {
  memset(&addr, 0, sizeof(addr));
  ink_inet_invalidate(addr);
}

/// Test for IP protocol.
/// @return @c true if the address is IP, @c false otherwise.
inline bool ink_inet_is_ip(sockaddr_storage const& addr) {
  return AF_INET == addr.ss_family || AF_INET6 == addr.ss_family;
}
/// Test for IP protocol.
/// Convenience overload.
/// @return @c true if the address is IP, @c false otherwise.
inline bool ink_inet_is_ip(sockaddr_storage const* addr) {
  return AF_INET == addr->ss_family || AF_INET6 == addr->ss_family;
}
/// Test for IPv4 protocol.
/// @return @c true if the address is IPv4, @c false otherwise.
inline bool ink_inet_is_ip4(sockaddr_storage const& addr) {
  return AF_INET == addr.ss_family;
}
/// Test for IPv4 protocol.
/// Convenience overload.
/// @return @c true if the address is IPv4, @c false otherwise.
inline bool ink_inet_is_ip4(sockaddr_storage const* addr) {
  return AF_INET == addr->ss_family;
}
/// Test for IPv6 protocol.
/// @return @c true if the address is IPv6, @c false otherwise.
inline bool ink_inet_is_ip6(sockaddr_storage const& addr) {
  return AF_INET6 == addr.ss_family;
}
/// Test for IPv6 protocol.
/// Convenience overload.
/// @return @c true if the address is IPv6, @c false otherwise.
inline bool ink_inet_is_ip6(sockaddr_storage const* addr) {
  return AF_INET6 == addr->ss_family;
}
/// @return @c true if the address families are compatible.
inline bool ink_inet_are_compatible(
  sockaddr_storage const* lhs, ///< Address to test.
  sockaddr_storage const* rhs  ///< Address to test.
) {
  return lhs->ss_family == rhs->ss_family;
}

// IP address casting.
// sa_cast to cast to sockaddr*.
// ss_cast to cast to sockaddr_storage*.
// ip4_cast converts to sockaddr_in (because that's effectively an IPv4 addr).
// ip6_cast converts to sockaddr_in6
inline sockaddr* ink_inet_sa_cast(sockaddr_storage* a) {
  return static_cast<sockaddr*>(static_cast<void*>(a));
}
inline sockaddr const* ink_inet_sa_cast(sockaddr_storage const* a) {
  return static_cast<sockaddr const*>(static_cast<void const*>(a));
}
inline sockaddr_storage* ink_inet_ss_cast(sockaddr* a) {
  return static_cast<sockaddr_storage*>(static_cast<void*>(a));
}
inline sockaddr_storage const* ink_inet_ss_cast(sockaddr const* a) {
  return static_cast<sockaddr_storage const*>(static_cast<void const*>(a));
}
inline sockaddr_storage* ink_inet_ss_cast(sockaddr_in* a) {
  return static_cast<sockaddr_storage*>(static_cast<void*>(a));
}
inline sockaddr_storage const* ink_inet_ss_cast(sockaddr_in const* a) {
  return static_cast<sockaddr_storage const*>(static_cast<void const*>(a));
}
inline sockaddr_storage* ink_inet_ss_cast(sockaddr_in6* a) {
  return static_cast<sockaddr_storage*>(static_cast<void*>(a));
}
inline sockaddr_storage const* ink_inet_ss_cast(sockaddr_in6 const* a) {
  return static_cast<sockaddr_storage const*>(static_cast<void const*>(a));
}
inline sockaddr_in* ink_inet_ip4_cast(sockaddr_storage* a) {
  return static_cast<sockaddr_in*>(static_cast<void*>(a));
}
inline sockaddr_in const* ink_inet_ip4_cast(sockaddr_storage const* a) {
  return static_cast<sockaddr_in const*>(static_cast<void const*>(a));
}
inline sockaddr_in& ink_inet_ip4_cast(sockaddr_storage& a) {
  return *static_cast<sockaddr_in*>(static_cast<void*>(&a));
}
inline sockaddr_in const& ink_inet_ip4_cast(sockaddr_storage const& a) {
  return *static_cast<sockaddr_in const*>(static_cast<void const*>(&a));
}
inline sockaddr_in6* ink_inet_ip6_cast(sockaddr_storage* a) {
  return static_cast<sockaddr_in6*>(static_cast<void*>(a));
}
inline sockaddr_in6 const* ink_inet_ip6_cast(sockaddr_storage const* a) {
  return static_cast<sockaddr_in6 const*>(static_cast<void const*>(a));
}
inline sockaddr_in6& ink_inet_ip6_cast(sockaddr_storage& a) {
  return *static_cast<sockaddr_in6*>(static_cast<void*>(&a));
}
inline sockaddr_in6 const& ink_inet_ip6_cast(sockaddr_storage const& a) {
  return *static_cast<sockaddr_in6 const*>(static_cast<void const*>(&a));
}
/** Get a reference to the port in an address.
    @note Because this is direct access, the port value is in network order.
    @see ink_inet_get_port for host order copy.
    @return A reference to the port value in an IPv4 or IPv6 address.
    @internal This is primarily for internal use but it might be handy for
    clients so it is exposed.
*/
inline uint16_t& ink_inet_port_cast(sockaddr_storage* ss) {
  static uint16_t dummy = 0;
  return AF_INET == ss->ss_family
    ? ink_inet_ip4_cast(ss)->sin_port
    : AF_INET6 == ss->ss_family
      ? ink_inet_ip6_cast(ss)->sin6_port
      : (dummy = 0)
    ;
}
/** Get a reference to the port in an address.
    @note Convenience overload.
    @see ink_inet_port_cast(sockaddr_storage* ss)
*/
inline uint16_t& ink_inet_port_cast(sockaddr_storage& ss) {
  return ink_inet_port_cast(&ss);
}

/** Access the IPv4 address.

    If this is not an IPv4 address a zero valued address is returned.
    @note This is direct access to the address so it will be in
    network order.

    @return A reference to the IPv4 address in @a addr.
*/
inline uint32_t& ink_inet_ip4_addr_cast(sockaddr_storage* addr) {
  static uint32_t dummy = 0;
  return ink_inet_is_ip4(addr)
    ? ink_inet_ip4_cast(addr)->sin_addr.s_addr
    : (dummy = 0)
    ;
}
/** Access the IPv4 address.

    If this is not an IPv4 address a zero valued address is returned.
    @note This is direct access to the address so it will be in
    network order.

    @return A reference to the IPv4 address in @a addr.
*/
inline uint32_t const& ink_inet_ip4_addr_cast(sockaddr_storage const* addr) {
  static uint32_t dummy = 0;
  return ink_inet_is_ip4(addr)
    ? ink_inet_ip4_cast(addr)->sin_addr.s_addr
    : static_cast<uint32_t const&>(dummy = 0)
    ;
}
/** Access the IPv6 address.

    If this is not an IPv6 address a zero valued address is returned.
    @note This is direct access to the address so it will be in
    network order.

    @return A reference to the IPv6 address in @a addr.
*/
inline in6_addr& ink_inet_ip6_addr_cast(sockaddr_storage* addr) {
  return ink_inet_ip6_cast(addr)->sin6_addr;
}
  

/// @name sockaddr_storage operators
//@{

/** Copy the address from @a src to @a dst if it's IP.
    This attempts to do a minimal copy based on the type of @a src.
    If @a src is not an IP address type it is @b not copied.
    @return @c true if @a src was an IP address, @c false otherwise.
*/
inline bool ink_inet_copy(
  sockaddr_storage* dst, ///< Destination object.
  sockaddr_storage const* src ///< Source object.
) {
  size_t n = 0;
  switch (src->ss_family) {
  case AF_INET: n = sizeof(sockaddr_in); break;
  case AF_INET6: n = sizeof(sockaddr_in6); break;
  }
  if (n) memcpy(dst, src, n);
  else ink_inet_invalidate(dst);
  return n != 0;
}
/** Copy the address from @a src to @a dst if it's IP.
    This attempts to do a minimal copy based on the type of @a src.
    If @a src is not an IP address type it is @b not copied.
    @note Convenience overload.
    @return @c true if @a src was an IP address, @c false otherwise.
*/
inline bool ink_inet_copy(
  sockaddr_storage& dst, ///< Destination object.
  sockaddr_storage const& src ///< Source object.
) {
  return ink_inet_copy(&dst, &src);
}

/** Compare two addresses.
    This works only for IP addresses (IPv4, IPv6). A comparison bewteen
    IPv4 and IPv6 will always return the IPv4 address as lesser.
    @return
      - -1 if @a lhs is less than @a rhs.
      - 0 if @a lhs is identical to @a rhs.
      - 1 if @a lhs is greater than @a rhs.
    @internal This looks like a lot of code for an inline but I think it
    should compile down quite a bit.
*/
inline int ink_inet_cmp(
  sockaddr_storage const* lhs, ///< Left hand operand.
  sockaddr_storage const* rhs ///< Right hand operand.
) {
  int zret = 0;
  uint16_t rtype = rhs->ss_family;
  uint16_t ltype = lhs->ss_family;

  if (AF_INET == ltype) {
    if (AF_INET == rtype) {
      zret = memcmp(
        &ink_inet_ip4_cast(lhs)->sin_addr,
        &ink_inet_ip4_cast(rhs)->sin_addr,
        sizeof(sockaddr_in::sin_addr)
      );
    } else if (AF_INET6 == rtype) {
      zret = -1; // IPv4 addresses are before IPv6
    } else {
      ink_assert(false); // Comparing an IPv4 address to non-IP.
    }
  } else if (AF_INET6 == ltype) {
    if (AF_INET == rtype) {
      zret = 1; // IPv6 always greater than IPv4
    } else if (AF_INET6 == rtype) {
      zret = memcmp(
        &ink_inet_ip6_cast(lhs)->sin6_addr,
        &ink_inet_ip6_cast(rhs)->sin6_addr,
        sizeof(sockaddr_in6::sin6_addr)
      );
    }
  } else {
    ink_assert(false); // Compare only works for IP presently.
  }

  return zret;
}
/** Compare two addresses.
    This works only for IP addresses (IPv4, IPv6). A comparison bewteen
    IPv4 and IPv6 will always return the IPv4 address as lesser.
    @note Convenience overload.
    @return
      - -1 if @a lhs is less than @a rhs.
      - 0 if @a lhs is identical to @a rhs.
      - 1 if @a lhs is greater than @a rhs.
    @internal This looks like a lot of code for an inline but I think it
    should compile down quite a bit.
*/
inline int ink_inet_cmp(
  sockaddr_storage const& lhs, ///< Left hand operand.
  sockaddr_storage const& rhs ///< Right hand operand.
) {
  return ink_inet_cmp(&lhs, &rhs);
}

/// Equality.
/// @return @c true if @a lhs and @a rhs are identical.
inline bool operator == (
  sockaddr_storage const& lhs, ///< Left operand.
  sockaddr_storage const& rhs  ///< Right operand.
) {
  return 0 == ink_inet_cmp(lhs, rhs);
}
/// Inequality.
/// @return @c false if @a lhs and @a rhs are identical.
inline bool operator != (
  sockaddr_storage const& lhs, ///< Left operand.
  sockaddr_storage const& rhs  ///< Right operand.
) {
  return 0 != ink_inet_cmp(lhs, rhs);
}
/// Less than.
/// @return @c true iff @a lhs < @a rhs.
inline bool operator <  (
  sockaddr_storage const& lhs, ///< Left operand.
  sockaddr_storage const& rhs  ///< Right operand.
) {
  return -1 == ink_inet_cmp(lhs, rhs);
}
/// Less than or equal.
/// @return @c true iff @a lhs < @a rhs.
inline bool operator <=  (
  sockaddr_storage const& lhs, ///< Left operand.
  sockaddr_storage const& rhs  ///< Right operand.
) {
  return 1 != ink_inet_cmp(lhs, rhs);
}
//@}

/// Get IP TCP/UDP port.
/// @return The port in host order for an IPv4 or IPv6 address,
/// or zero if neither.
inline uint16_t ink_inet_get_port(
  sockaddr_storage const* addr ///< Address with port.
) {
  // We can discard the const because this function returns
  // by value.
  return ntohs(ink_inet_port_cast(const_cast<sockaddr_storage&>(*addr)));
}
/// Get IP TCP/UDP port.
/// @note Convenience overload.
/// @return The port in host order for an IPv4 or IPv6 address,
/// or zero if neither.
inline uint16_t ink_inet_get_port(
  sockaddr_storage const& addr ///< Address with port.
) {
  return ink_inet_get_port(&addr);
}

/** Extract the IPv4 address.
    @return Host order IPv4 address.
*/
inline uint32_t ink_inet_get_ip4_addr(
  sockaddr_storage const* addr ///< Address object.
) {
  return ntohl(ink_inet_ip4_addr_cast(const_cast<sockaddr_storage*>(addr)));
}

/// Write IPv4 data to a @c sockaddr_storage.
inline void ink_inet_ip4_set(
  sockaddr_storage* ss, ///< Destination storage.
  uint32_t ip4, ///< address, IPv4 network order.
  uint16_t port = 0 ///< port, network order.
) {
  sockaddr_in* sin = ink_inet_ip4_cast(ss);
  memset(sin, 0, sizeof(*sin));
  sin->sin_family = AF_INET;
  memcpy(&(sin->sin_addr), &ip4, sizeof(ip4));
  sin->sin_port = port;
}
/// Write IPv4 data to a @c sockaddr_storage.
/// @note Convenience overload.
inline void ink_inet_ip4_set(
  sockaddr_storage& ss, ///< Destination storage.
  uint32_t ip4, ///< address, IPv4 network order.
  uint16_t port = 0 ///< port, network order.
) {
  ink_inet_ip4_set(&ss, ip4, port);
}

/** Just the address.
    In some cases we want to store just the address and not the
    ancillary information (such as port, or flow data) in
    @c sockaddr_storage.
    @note This is not easily used as an address for system calls.
*/
struct InkInetAddr {
  typedef InkInetAddr self; ///< Self reference type.

  /// Default construct (invalid address).
  InkInetAddr() : _family(AF_UNSPEC) {}
  /// Construct as IPv4 @a addr.
  explicit InkInetAddr(
    uint32_t addr ///< Address to assign.
  ) : _family(AF_INET) {
    _addr._ip4 = addr;
  }
  /// Construct from @c sockaddr_storage.
  explicit InkInetAddr(sockaddr_storage const& addr) { this->assign(&addr); }
  /// Construct from @c sockaddr_storage.
  explicit InkInetAddr(sockaddr_storage const* addr) { this->assign(addr); }

  /// Assign sockaddr storage.
  self& assign(sockaddr_storage const* addr) {
    _family = addr->ss_family;
    if (ink_inet_is_ip4(addr)) {
      _addr._ip4 = ink_inet_ip4_addr_cast(addr);
    } else if (ink_inet_is_ip6(addr)) {
      memcpy(&_addr._ip6, &ink_inet_ip6_cast(addr)->sin6_addr, INK_IP6_SIZE);
    } else {
      _family = AF_UNSPEC;
    }
    return *this;
  }

  /// Assign an IPv4 address.
# if 0
  self& operator=(uint32_t addr) {
    _family = AF_INET;
    _addr._ip4 = addr;
    return *this;
  }
# endif

  /// Equality.
  bool operator==(self const& that) {
    return _family == AF_INET
      ? (that._family == AF_INET && _addr._ip4 == that._addr._ip4)
      : _family == AF_INET6
        ? (that._family == AF_INET6
          && 0 == memcmp(_addr._ip6, that._addr._ip6, INK_IP6_SIZE)
          )
        : (_family = AF_UNSPEC && that._family == AF_UNSPEC)
    ;
  }

  /// Inequality.
  bool operator!=(self const& that) {
    return ! (*this == that);
  }

  /// Test for validity.
  bool isValid() const { return _family == AF_INET || _family == AF_INET6; }

  uint8_t _family; ///< Protocol family.
  uint8_t _pad[3]; ///< Pad it out.
  /// Address data.
  union {
    uint32_t _ip4; ///< As IPv4 address.
    uint8_t _ip6[INK_IP6_SIZE]; ///< As IPv6 address.
  } _addr;
};

#endif // _ink_inet.h
