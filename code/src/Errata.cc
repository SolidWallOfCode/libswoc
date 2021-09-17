// SPDX-License-Identifier: Apache-2.0
// Copyright Network Geographics 2014
/** @file

    Errata implementation.
 */

#include <iostream>
#include <sstream>
#include <algorithm>
#include <memory.h>
#include "swoc/Errata.h"
#include "swoc/bwf_ex.h"
#include "swoc/bwf_std.h"

using swoc::MemArena;
using std::string_view;
using namespace std::literals;
using namespace swoc::literals;

namespace swoc { inline namespace SWOC_VERSION_NS {
/** List of sinks for abandoned erratum.
 */
namespace {
std::vector<Errata::Sink::Handle> Sink_List;
}

std::string_view Errata::DEFAULT_GLUE{"\n", 1};

/** This is the implementation class for Errata.

    It holds the actual messages and is treated as a passive data object with nice constructors.
*/
string_view
Errata::Data::localize(string_view src) {
  auto span = _arena.alloc(src.size());
  memcpy(span.data(), src.data(), src.size());
  return span.view();
}

/* ----------------------------------------------------------------------- */
// methods for Errata

Errata::Severity Errata::DEFAULT_SEVERITY(1);
Errata::Severity Errata::FAILURE_SEVERITY(1);
swoc::MemSpan<TextView> Errata::SEVERITY_NAME;

Errata::~Errata() {
  if (_data) {
    if (!_data->empty()) {
      for (auto &f : Sink_List) {
        (*f)(*this);
      }
    }
    this->reset();
  }
}

Errata&
Errata::note(code_type const& code){
  return this->note("{}"_sv, code);
}

Errata::Data *
Errata::data() {
  if (!_data) {
    MemArena arena{512};
    _data = arena.make<Data>(std::move(arena));
  }
  return _data;
}

Errata&
Errata::note_localized(std::string_view const& text) {
  auto d = this->data();
  Annotation *n = d->_arena.make<Annotation>(text);
  n->_level = d->_level;
  d->_notes.prepend(n);
  return *this;
}

MemSpan<char>
Errata::alloc(size_t n) {
  return this->data()->_arena.alloc(n).rebind<char>();
}

Errata&
Errata::note(const self_type& that) {
  for (auto const& m : that) {
    this->note(m._text);
  }
  return *this;
}

Errata&
Errata::clear() {
  if (_data) {
    _data->_notes.clear();
  }
  return *this;
}

void
Errata::register_sink(Sink::Handle const& s) {
  Sink_List.push_back(s);
}

std::ostream&
Errata::write(std::ostream& out) const {
  string_view lead;
  for (auto& m : *this) {
    out << lead << m._text << std::endl;
    if (0 == lead.size()) {
      lead = "  "_sv;
    }
  }
  return out;
}

BufferWriter&
bwformat(BufferWriter& bw, bwf::Spec const& spec, Errata::Severity level) {
  if (level < Errata::SEVERITY_NAME.size()) {
    bwformat(bw, spec, Errata::SEVERITY_NAME[level]);
  } else {
    bwformat(bw, spec, level._raw);
  }
  return bw;
}

BufferWriter&
bwformat(BufferWriter& bw, bwf::Spec const&, Errata const& errata) {

  bw.print("{} ", errata.severity());

  if (errata.code()) {
    bw.print("[{0:s} {0:d}] ", errata.code());
  }


  for (auto& m : errata) {
    bw.print("{}{}\n", swoc::bwf::Pattern{int(m.level()), "  "}, m.text());
  }
  return bw;
}

std::ostream&
operator<<(std::ostream& os, Errata const& err) {
  return err.write(os);
}

}} // namespace swoc
