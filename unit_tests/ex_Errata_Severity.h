/** @file

    Errata initialization for testing.
    Useful as an examle.

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

#include "swoc/Errata.h"

static constexpr swoc::Errata::Severity ERRATA_DBG{0};
static constexpr swoc::Errata::Severity ERRATA_DIAG{1};
static constexpr swoc::Errata::Severity ERRATA_INFO{2};
static constexpr swoc::Errata::Severity ERRATA_WARN{3};
static constexpr swoc::Errata::Severity ERRATA_ERROR{4};

inline swoc::Errata & Dbg(swoc::Errata & erratum, std::string_view text) {
  return erratum.note(ERRATA_DBG, text);
}

inline swoc::Errata & Diag(swoc::Errata & erratum, std::string_view text) {
  return erratum.note(ERRATA_DIAG, text);
}

inline swoc::Errata & Info(swoc::Errata & erratum, std::string_view text) {
  return erratum.note(ERRATA_INFO, text);
}

inline swoc::Errata & Warn(swoc::Errata & erratum, std::string_view text) {
  return erratum.note(ERRATA_WARN, text);
}

inline swoc::Errata & Error(swoc::Errata & erratum, std::string_view text) {
  return erratum.note(ERRATA_ERROR, text);
}
