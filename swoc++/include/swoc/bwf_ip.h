/** @file

    BufferWriter formatting for IP addresses.

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

#pragma once

#include <iosfwd>
#include "swoc/bwf_base.h"
#include "swoc/swoc_ip.h"
#include <netinet/in.h>

namespace swoc
{
// All of these expect the address to be in network order.
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, sockaddr const *addr);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, in6_addr const &addr);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, sockaddr const *addr);
// Use class information for ordering.
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IP4Addr const &addr);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IP6Addr const &addr);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IPAddr const &addr);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IP4Range const &Range);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IP6Range const &Range);
BufferWriter &bwformat(BufferWriter &w, bwf::Spec const &spec, IPRange const &Range);

inline BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, IPEndpoint const &addr) {
  return bwformat(w, spec, &addr.sa);
}

/// Buffer space sufficient for printing any basic IP address type.
static const size_t IP_STREAM_SIZE = 80;

} // namespace swoc

namespace std {
inline ostream &
operator<<(ostream &s, swoc::IP4Addr const &addr) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, addr);
}

inline ostream &
operator<<(ostream &s, swoc::IP6Addr const &addr) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, addr);
}

inline ostream &
operator<<(ostream &s, swoc::IPAddr const &addr) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, addr);
}

inline ostream &
operator<<(ostream &s, swoc::IP4Range const &Range) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, Range);
}

inline ostream &
operator<<(ostream &s, swoc::IP6Range const &Range) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, Range);
}

inline ostream &
operator<<(ostream &s, swoc::IPRange const &Range) {
  swoc::LocalBufferWriter<swoc::IP_STREAM_SIZE> w;
  return s << bwformat(w, swoc::bwf::Spec::DEFAULT, Range);
}
} // namespace std

