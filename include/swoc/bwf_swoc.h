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
template <typename T>
auto
tag_label(BufferWriter &w, const bwf::Spec &, const swoc::meta::CaseArg_0 &) -> BufferWriter &
{
  return w;
}

template <typename T>
auto
tag_label(BufferWriter &w, const bwf::Spec &, const swoc::meta::CaseArg_1 &) -> decltype(T::label, []() -> BufferWriter & {})
{
  return w.print("{}", T::label);
}
template <typename T>
inline BufferWriter &
tag_label(BufferWriter &w, BWFSpec const &spec)
{
  return tag_label<T>(w, spec, swoc::meta::CaseArg_Final);
}

template <typename T>
auto
tag_label(std::ostream &, const swoc::meta::CaseArg_0 &) -> std::ostream &
{
  return w;
}

template <typename T>
auto
tag_label(std::ostream &w, const swoc::meta::CaseArg_1 &) -> decltype(T::label, []() -> std::ostream & {})
{
  return w << T::label;
}

template <typename T>
inline std::ostream &
tag_label(std::ostream &w)
{
  return tag_label<T>(w, swoc::meta::CaseArg_Final);
}
} // namespace detail

template <intmax_t N, typename C, typename T>
BufferWriter &
bwformat(BufferWriter &w, bwf::Spec const &spec, Scalar<N, C, T> const &x)
{
  bwformat(w, spec, x.value());
  return spec.has_numeric_type() ? w : ts::detail::tag_label<T>(w, spec);
}

} // namespace swoc

namespace std
{
template <intmax_t N, typename C, typename T>
ostream &
operator<<(ostream &s, ts::Scalar<N, C, T> const &x)
{
  s << x.value();
  return ts::detail::tag_label<T>(s, b);
}

