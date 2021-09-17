// SPDX-License-Identifier: Apache-2.0
/** @file

    Errata unit tests.
*/

#include <memory>
#include <errno.h>
#include "swoc/Errata.h"
#include "swoc/swoc_file.h"
#include "catch.hpp"

using swoc::Errata;
using swoc::Rv;
using Severity = swoc::Errata::Severity;
using namespace std::literals;
using namespace swoc::literals;

static constexpr swoc::Errata::Severity ERRATA_DBG{0};
static constexpr swoc::Errata::Severity ERRATA_DIAG{1};
static constexpr swoc::Errata::Severity ERRATA_INFO{2};
static constexpr swoc::Errata::Severity ERRATA_WARN{3};
static constexpr swoc::Errata::Severity ERRATA_ERROR{4};

std::array<swoc::TextView, 5> Severity_Names { {
  "Debug", "Diag", "Info", "Warn", "Error"
}};

// Call from unit test main before starting tests.
void test_Errata_init() {
  swoc::Errata::DEFAULT_SEVERITY = ERRATA_ERROR;
  swoc::Errata::FAILURE_SEVERITY = ERRATA_WARN;
  swoc::Errata::SEVERITY_NAMES = swoc::MemSpan<swoc::TextView>(Severity_Names.data(), Severity_Names.size());
}

Errata
Noteworthy(std::string_view text)
{
  return Errata{ERRATA_INFO, text};
}

Errata
cycle(Errata &erratum)
{
  return std::move(erratum.note("Note well, young one!"));
}

TEST_CASE("Errata copy", "[libswoc][Errata]")
{
  auto notes = Noteworthy("Evil Dave Rulz.");
  REQUIRE(notes.length() == 1);
  REQUIRE(notes.begin()->text() == "Evil Dave Rulz.");

  notes = cycle(notes);
  REQUIRE(notes.length() == 2);

  Errata erratum;
  REQUIRE(erratum.length() == 0);
  erratum.note("Diagnostics");
  REQUIRE(erratum.length() == 1);
  erratum.note("Information");
  REQUIRE(erratum.length() == 2);

  // Test internal allocation boundaries.
  notes.clear();
  std::string_view text{"0123456789012345678901234567890123456789"};
  for (int i = 0; i < 50; ++i) {
    notes.note(text);
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
  zret = Errata(std::error_code(EINVAL, std::generic_category()), ERRATA_ERROR, "This is an error");

  {
    auto & [result, erratum] = zret;

    REQUIRE(erratum.length() == 1);
    REQUIRE(erratum.severity() == ERRATA_ERROR);

    REQUIRE(result == 17);
    zret = 38;
    REQUIRE(result == 38); // reference binding, update.
  }

  {
    auto && [result, erratum] = zret;

    REQUIRE(erratum.length() == 1);
    REQUIRE(erratum.severity() == ERRATA_ERROR);

    REQUIRE(result == 38);
    zret = 56;
    REQUIRE(result == 56); // reference binding, update.
  }

  auto test = [](Severity expected_severity, Rv<int> const &rvc) {
    auto const &[cv_result, cv_erratum] = rvc;
    REQUIRE(cv_erratum.length() == 1);
    REQUIRE(cv_erratum.severity() == expected_severity);
    REQUIRE(cv_result == 56);
  };

  {
    auto const &[result, erratum] = zret;
    REQUIRE(result == 56);

    test(ERRATA_ERROR, zret); // invoke it.
  }


  zret.clear();
  REQUIRE(zret.result() == 56);

  {
    auto const &[result, erratum] = zret;
    REQUIRE(result == 56);
    REQUIRE(erratum.length() == 0);
  }

  zret.note("Diagnostics");
  REQUIRE(zret.errata().length() == 1);
  zret.note("Information");
  REQUIRE(zret.errata().length() == 2);
  zret.note("Warning");
  REQUIRE(zret.errata().length() == 3);
  zret.note("Error");
  REQUIRE(zret.errata().length() == 4);
  REQUIRE(zret.result() == 56);

  test(ERRATA_DIAG, Rv<int>{56, Errata(ERRATA_DIAG, "Test rvalue diag")});
  test(ERRATA_INFO, Rv<int>{56, Errata(ERRATA_INFO, "Test rvalue info")});
  test(ERRATA_WARN, Rv<int>{56, Errata(ERRATA_WARN, "Test rvalue warn")});
  test(ERRATA_ERROR, Rv<int>{56, Errata(ERRATA_ERROR, "Test rvalue error")});

  // Test the note overload that takes an Errata.
  zret.clear();
  REQUIRE(zret.result() == 56);
  REQUIRE(zret.errata().length() == 0);
  zret = Errata{ERRATA_INFO, "Information"};
  REQUIRE(ERRATA_INFO == zret.errata().severity());
  REQUIRE(zret.errata().length() == 1);

  Errata e1{ERRATA_DBG, "Debug"};
  zret.note(e1);
  REQUIRE(zret.errata().length() == 2);
  REQUIRE(ERRATA_INFO == zret.errata().severity());

  Errata e2{ERRATA_DBG, "Debug"};
  zret.note(std::move(e2));
  REQUIRE(zret.errata().length() == 3);
  REQUIRE(e2.length() == 0);

  // Now try it on a non-copyable object.
  ThingHandle handle{new Thing};
  Rv<ThingHandle> thing_rv;

  handle->s = "other"; // mark it.
  thing_rv  = std::move(handle);
  thing_rv = Errata(ERRATA_WARN, "This is a warning");

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

TEST_CASE("Errata example", "[libswoc][Errata]") {
  swoc::LocalBufferWriter<2048> w;
  std::error_code ec;
  swoc::file::path path("does-not-exist.txt");
  auto content = swoc::file::load(path, ec);
  REQUIRE(false == !ec); // it is expected the load will fail.
  Errata errata{ec, ERRATA_ERROR, R"(Failed to open file "{}")", path};
  w.print("{}", errata);
  REQUIRE(w.size() > 0);
  REQUIRE(w.view().starts_with("Error") == true);
  REQUIRE(w.view().find("enoent") != swoc::TextView::npos);
}
