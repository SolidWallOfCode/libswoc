/** @file
 * @c BufferWriter for a @c MemArena.
 *
 * Copyright 2019, Oath Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "swoc/ArenaWriter.h"

namespace swoc
{
ArenaWriter &
ArenaWriter::write(char c)
{
  if (_attempted >= _capacity) {
    this->realloc(_attempted + 1);
  }
  this->super_type::write(c);
  return *this;
}

ArenaWriter &
ArenaWriter::write(void const *data, size_t n)
{
  if (n + _attempted > _capacity) {
    this->realloc(n + _attempted);
  }
  this->super_type::write(data, n);
  return *this;
}

bool
ArenaWriter::commit(size_t n)
{
  if (_attempted + n > _capacity) {
    this->realloc(_attempted + n);
    return false;
  }
  return this->super_type::commit(n);
}

void
ArenaWriter::realloc(size_t n)
{
  auto text                    = this->view(); // Current data.
  auto span                    = _arena.require(n).remnant().rebind<char>();
  const_cast<char *&>(_buffer) = span.data();
  _capacity                    = span.size();
  memcpy(_buffer, text.data(), text.size());
}

} // namespace swoc
