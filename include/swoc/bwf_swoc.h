/** @file

    BufferWriter formatters for base types in libswoc.

    This is primarily for classes on which BufferWriter formatting depends - those formatters
    can't be defined before BufferWriter formatting is.

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

#include <ostream>
#include "swoc/bwf_base.h"

namespace swoc
{
namespace detail
{
  template <typename T> auto tag_label(BufferWriter &w, const bwf::Spec &, const meta::CaseArg_0 &) -> void {}

  template <typename T>
  auto
  tag_label(BufferWriter &w, const bwf::Spec &, const meta::CaseArg_1 &) -> decltype(T::label, meta::CaseVoidFunc())
  {
    w.print("{}", T::label);
  }
  template <typename T>
  inline BufferWriter &
  tag_label(BufferWriter &w, bwf::Spec const &spec)
  {
    tag_label<T>(w, spec, meta::CaseArg);
    return w;
  }

  template <typename T> auto tag_label(std::ostream &, const meta::CaseArg_0 &) -> void {}

  template <typename T>
  auto
  tag_label(std::ostream &w, const meta::CaseArg_1 &) -> decltype(T::label, meta::CaseVoidFunc())
  {
    w << T::label;
  }

  template <typename T>
  inline std::ostream &
  tag_label(std::ostream &w)
  {
    tag_label<T>(w, meta::CaseArg);
    return w;
  }
} // namespace detail

template <intmax_t N, typename C, typename T>
BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, Scalar<N, C, T> const &x)
{
  bwformat(w, spec, x.value());
  return spec.has_numeric_type() ? w : detail::tag_label<T>(w, spec);
}

} // namespace swoc

namespace std {
template<intmax_t N, typename C, typename T>
ostream &
operator<<(ostream &s, swoc::Scalar<N, C, T> const &x) {
  s << x.value();
  return swoc::detail::tag_label<T>(s);
}

} // namespace std
