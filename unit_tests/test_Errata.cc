/** @file

    Errata unit tests.

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

#include <memory>
#include "swoc/Errata.h"
#include "ex_Errata_Severity.h"
#include "catch.hpp"

using swoc::Errata;
using swoc::Rv;
using Severity = swoc::Errata::Severity;
using namespace std::literals;

Errata
Noteworthy(std::string_view text)
{
  Errata notes;
  Info(notes, text);
  return notes;
}

Errata
cycle(Errata &erratum)
{
  Info(erratum, "Note well, young one!");
  return erratum;
}

TEST_CASE("Errata copy", "[libswoc][Errata]")
{
  auto notes = Noteworthy("Evil Dave Rulz.");
  REQUIRE(notes.length() == 1);
  REQUIRE(notes.begin()->text() == "Evil Dave Rulz.");

  notes = cycle(notes);
  REQUIRE(notes.length() == 2);

  Errata erratum;
  erratum.clear();
  REQUIRE(erratum.length() == 0);
  Diag(erratum, "Diagnostics");
  REQUIRE(erratum.length() == 1);
  Info(erratum, "Information");
  REQUIRE(erratum.length() == 2);
  Warn(erratum, "Warning");
  REQUIRE(erratum.length() == 3);
  Error(erratum, "Error");
  REQUIRE(erratum.length() == 4);

  // Test internal allocation boundaries.
  notes.clear();
  std::string_view text{"0123456789012345678901234567890123456789"};
  for (int i = 0; i < 50; ++i) {
    Info(notes, text);
  }
  REQUIRE(notes.length() == 50);
  REQUIRE(notes.begin()->text() == text);
  bool match_p = true;
  for (auto &&note : notes) {
    if (note.text() != text) {
      match_p = false;
      break;
    }
  }
  REQUIRE(match_p);
};

TEST_CASE("Rv", "[libswoc][Errata]")
{
  Rv<int> zret;
  struct Thing {
    char const *s = "thing";
  };
  using ThingHandle = std::unique_ptr<Thing>;

  zret = 17;
  zret.note(ERRATA_ERROR, "This is an error");

  auto [result, erratum] = zret;

  REQUIRE(erratum.length() == 1);
  REQUIRE(erratum.severity() == ERRATA_ERROR);

  REQUIRE(result == 17);
  zret = 38;
  REQUIRE(result == 17); // Local copy binding, no update.

  auto &[r_result, r_erratum] = zret;

  REQUIRE(r_result == 38);
  zret = 56;
  REQUIRE(r_result == 56); // reference binding, update.

  auto const &[cr_result, cr_erratum] = zret;
  REQUIRE(cr_result == 56);

  auto test = [](Severity expected_severity, Rv<int> const &rvc) {
    auto const &[cv_result, cv_erratum] = rvc;
    REQUIRE(cv_erratum.length() == 1);
    REQUIRE(cv_erratum.severity() == expected_severity);
    REQUIRE(cv_result == 56);
  };

  test(ERRATA_ERROR, zret); // invoke it.

  zret.clear();
  auto const &[cleared_result, cleared_erratum] = zret;
  REQUIRE(cleared_result == 56);
  REQUIRE(cleared_erratum.length() == 0);
  Diag(zret, "Diagnostics");
  REQUIRE(zret.errata().length() == 1);
  Info(zret, "Information");
  REQUIRE(zret.errata().length() == 2);
  Warn(zret, "Warning");
  REQUIRE(zret.errata().length() == 3);
  Error(zret, "Error");
  REQUIRE(zret.errata().length() == 4);
  REQUIRE(zret.result() == 56);

  test(ERRATA_DIAG, Rv<int>{56}.note(ERRATA_DIAG, "Test rvalue diag"));
  test(ERRATA_INFO, Rv<int>{56}.note(ERRATA_INFO, "Test rvalue info"));
  test(ERRATA_WARN, Rv<int>{56}.note(ERRATA_WARN, "Test rvalue warn"));
  test(ERRATA_ERROR, Rv<int>{56}.note(ERRATA_ERROR, "Test rvalue error"));

  // Test the note overload that takes an Errata.
  zret.clear();
  REQUIRE(zret.result() == 56);
  REQUIRE(zret.errata().length() == 0);
  Errata errata;
  Info(errata, "Information");
  zret.note(errata);
  test(ERRATA_INFO, zret);
  REQUIRE(errata.length() == 1);
  zret.note(std::move(errata));
  REQUIRE(zret.errata().length() == 2);
  REQUIRE(errata.length() == 0);

  // Now try it on a non-copyable object.
  ThingHandle handle{new Thing};
  Rv<ThingHandle> thing_rv;

  handle->s = "other"; // mark it.
  thing_rv  = std::move(handle);
  thing_rv.note(ERRATA_WARN, "This is a warning");

  auto &&[tr1, te1]{thing_rv};
  REQUIRE(te1.length() == 1);
  REQUIRE(te1.severity() == ERRATA_WARN);

  ThingHandle other{std::move(tr1)};
  REQUIRE(tr1.get() == nullptr);
  REQUIRE(thing_rv.result().get() == nullptr);
  REQUIRE(other->s == "other"sv);

  auto maker = []() -> Rv<ThingHandle> {
    ThingHandle handle = std::make_unique<Thing>();
    handle->s          = "made";
    return {std::move(handle)};
  };

  auto &&[tr2, te2]{maker()};
  REQUIRE(tr2->s == "made"sv);
};
