/** @file

  @section license License

  Licensed to the Apache Software Foundation (ASF) under one or more contributor license agreements.
  See the NOTICE file distributed with this work for additional information regarding copyright
  ownership.  The ASF licenses this fileN to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance with the License.  You may obtain a
  copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software distributed under the License
  is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
  or implied. See the License for the specific language governing permissions and limitations under
  the License.
 */

/*
  http://www.isthe.com/chongo/tech/comp/fnv/

  Currently implemented FNV-1a 32bit and FNV-1a 64bit
 */

#pragma once

#include <cstdint>

struct Hash32_FNV {
  using self_type = Hash32_FNV;
  static constexpr uint32_t INIT = 0x811c9dc5u;

  Hash32_FNV();

  template <typename XF> self_type &update(const void *data, size_t len, const XF &xf);
  self_type &update(const void *data, size_t len);

  self_type & final();
  self_type &clear();

  template <typename XF> uint32_t hash_immediate(const void *data, size_t len, const XF &xf);

  operator uint32_t() const;

private:
  uint32_t hval;
};

struct Hash64_FNV {
  using self_type = Hash64_FNV;
  static constexpr uint64_t INIT = 0xcbf29ce484222325ull;

  Hash64_FNV();

  template <typename XF> self_type &update(const void *data, size_t len, const XF &xf);
  self_type &update(const void *data, size_t len);

  self_type & final();
  self_type &clear();

  template <typename XF> uint64_t hash_immediate(const void *data, size_t len, const XF &xf);

  operator uint64_t() const;

private:
  uint64_t hval;
};

// ----------
// Implementation

inline Hash32_FNV::Hash32_FNV()
{
  this->clear();
}

inline Hash32_FNV::operator uint32_t() const
{
  return hval;
}

inline auto
Hash32_FNV::final() -> self_type &
{
  return *this;
}

inline auto
Hash32_FNV::clear() -> self_type &
{
  hval = INIT;
  return *this;
}

template <typename XF>
auto
Hash32_FNV::update(const void *data, size_t len, const XF &xf) -> self_type &
{
  const uint8_t *bp = static_cast<const uint8_t *>(data);
  const uint8_t *be = bp + len;

  for (; bp < be; ++bp) {
    hval ^= static_cast<uint32_t>(xf(*bp));
    hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
  }
  return *this;
}

inline auto
Hash32_FNV::update(const void *data, size_t len) -> self_type &
{
  return this->update(data, len, [](uint8_t c) { return c; });
}

template <typename XF>
uint32_t
Hash32_FNV::hash_immediate(const void *data, size_t len, const XF &xf)
{
  return this->update(data, len, xf).final();
}

// ---

inline Hash64_FNV::Hash64_FNV()
{
  this->clear();
}

inline Hash64_FNV::operator uint64_t() const
{
  return hval;
}

inline auto
Hash64_FNV::clear() -> self_type &
{
  hval = INIT;
  return *this;
}

inline auto
Hash64_FNV::final() -> self_type &
{
  return *this;
}

template <typename XF>
auto
Hash64_FNV::update(const void *data, size_t len, const XF &xf) -> self_type &
{
  const uint8_t *bp = static_cast<const uint8_t *>(data);
  const uint8_t *be = bp + len;

  for (; bp < be; ++bp) {
    hval ^= static_cast<uint64_t>(xf(*bp));
    hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) + (hval << 8) + (hval << 40);
  }
  return *this;
}

inline auto
Hash64_FNV::update(const void *data, size_t len) -> self_type &
{
  return this->update(data, len, [](uint8_t c) { return c; });
}

template <typename XF>
uint64_t
Hash64_FNV::hash_immediate(const void *data, size_t len, const XF &xf)
{
  return this->update(data, len, xf).final();
}
