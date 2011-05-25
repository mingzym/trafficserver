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

#include "libts.h"

# if 0
#if defined(darwin)
extern "C"
{
  struct hostent *gethostbyname_r(const char *name, struct hostent *result, char *buffer, int buflen, int *h_errnop);
  struct hostent *gethostbyaddr_r(const char *name, size_t size, int type,
                                  struct hostent *result, char *buffer, int buflen, int *h_errnop);
}
#endif


struct hostent *
ink_gethostbyname_r(char *hostname, ink_gethostbyname_r_data * data)
{
#ifdef RENTRENT_GETHOSTBYNAME
  struct hostent *r = gethostbyname(hostname);
  if (r)
    data->ent = *r;
  data->herrno = errno;

#else //RENTRENT_GETHOSTBYNAME
#if GETHOSTBYNAME_R_GLIBC2

  struct hostent *addrp = NULL;
  int res = gethostbyname_r(hostname, &data->ent, data->buf,
                            INK_GETHOSTBYNAME_R_DATA_SIZE, &addrp,
                            &data->herrno);
  struct hostent *r = NULL;
  if (!res && addrp)
    r = addrp;

#else
  struct hostent *r = gethostbyname_r(hostname, &data->ent, data->buf,
                                      INK_GETHOSTBYNAME_R_DATA_SIZE,
                                      &data->herrno);
#endif
#endif
  return r;
}

struct hostent *
ink_gethostbyaddr_r(char *ip, int len, int type, ink_gethostbyaddr_r_data * data)
{
#if GETHOSTBYNAME_R_GLIBC2
  struct hostent *r = NULL;
  struct hostent *addrp = NULL;
  int res = gethostbyaddr_r((char *) ip, len, type, &data->ent, data->buf,
                            INK_GETHOSTBYNAME_R_DATA_SIZE, &addrp,
                            &data->herrno);
  if (!res && addrp)
    r = addrp;
#else
#ifdef RENTRENT_GETHOSTBYADDR
  struct hostent *r = gethostbyaddr((const void *) ip, len, type);

#else
  struct hostent *r = gethostbyaddr_r((char *) ip, len, type, &data->ent,
                                      data->buf,
                                      INK_GETHOSTBYNAME_R_DATA_SIZE,
                                      &data->herrno);
#endif
#endif //LINUX
  return r;
}

unsigned int
host_to_ip(char *hostname)
{
  struct hostent *he;

  he = gethostbyname(hostname);
  if (he == NULL)
    return INADDR_ANY;

  return *(unsigned int *) he->h_addr;
}

uint32_t
ink_inet_addr(const char *s)
{
  uint32_t u[4];
  uint8_t *pc = (uint8_t *) s;
  int n = 0;
  uint32_t base = 10;

  while (n < 4) {

    u[n] = 0;
    base = 10;

    // handle hex, octal

    if (*pc == '0') {
      if (*++pc == 'x' || *pc == 'X')
        base = 16, pc++;
      else
        base = 8;
    }
    // handle hex, octal, decimal

    while (*pc) {
      if (ParseRules::is_digit(*pc)) {
        u[n] = u[n] * base + (*pc++ - '0');
        continue;
      }
      if (base == 16 && ParseRules::is_hex(*pc)) {
        u[n] = u[n] * 16 + ParseRules::ink_tolower(*pc++) - 'a' + 10;
        continue;
      }
      break;
    }

    n++;
    if (*pc == '.')
      pc++;
    else
      break;
  }

  if (*pc && !ParseRules::is_wslfcr(*pc))
    return htonl((uint32_t) - 1);

  switch (n) {
  case 1:
    return htonl(u[0]);
  case 2:
    if (u[0] > 0xff || u[1] > 0xffffff)
      return htonl((uint32_t) - 1);
    return htonl((u[0] << 24) | u[1]);
  case 3:
    if (u[0] > 0xff || u[1] > 0xff || u[2] > 0xffff)
      return htonl((uint32_t) - 1);
    return htonl((u[0] << 24) | (u[1] << 16) | u[2]);
  case 4:
    if (u[0] > 0xff || u[1] > 0xff || u[2] > 0xff || u[3] > 0xff)
      return htonl((uint32_t) - 1);
    return htonl((u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3]);
  }
  return htonl((uint32_t) - 1);
}
# endif

const char *ink_inet_ntop(
  sockaddr_storage const* addr,
  char *dst, size_t size
) {
  void const* ptr = NULL;

  switch (addr->ss_family) {
  case AF_INET:
    ptr = &(ink_inet_ip4_cast(addr)->sin_addr);
    break;
  case AF_INET6:
    ptr = &(ink_inet_ip6_cast(addr)->sin6_addr);
    break;
  default:
    snprintf(dst, size, "Bad address type: %d", addr->ss_family);
    break;
  }

  return ptr ?
    inet_ntop(addr->ss_family, ptr, dst, size)
    : dst
    ;
}

const char *ink_inet_nptop(
  const sockaddr_storage *addr,
  char *dst, size_t size
) {
  char buff[INET6_ADDRSTRLEN];
  snprintf(dst, size, "%s:%u",
    ink_inet_ntop(addr, buff, sizeof(buff)),
    ink_inet_get_port(addr)
  );
  return dst;
}

int ink_inet_pton(char const* text, sockaddr_storage* ss) {
  int zret = -1;
  addrinfo hints; // [out]
  addrinfo *ai; // [in]

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_flags = AI_NUMERICHOST|AI_PASSIVE;
  if (0 == (zret = getaddrinfo(text, 0, &hints, &ai))) {
    if (ss) {
      if (ink_inet_copy(ss, ink_inet_ss_cast(ai->ai_addr)))
        zret = 0;
    } else if (ink_inet_is_ip(ss)) {
      zret = 0;
    }
    freeaddrinfo(ai);
  }
  return zret;
}

uint32_t ink_inet_hash(sockaddr_storage const* ip) {
  uint32_t zret = 0;
  uint32_t a4;
  switch (ip->ss_family) {
  case AF_INET:
    a4 = ink_inet_ip4_cast(ip)->sin_addr.s_addr;
    zret =  (((_client_ip >> 16)^_client_ip^_ip^(_ip>>16))&0xFFFF)
}
