// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file

    Example use of IPSpace for property mapping.
*/

#include "catch.hpp"

#include <memory>
#include <limits>

#include <swoc/TextView.h>
#include <swoc/swoc_ip.h>
#include <swoc/bwf_ip.h>
#include <swoc/swoc_file.h>

using namespace std::literals;
using namespace swoc::literals;
using swoc::TextView;
using swoc::IPEndpoint;

using swoc::IP4Addr;
using swoc::IP4Range;

using swoc::IP6Addr;
using swoc::IP6Range;

using swoc::IPAddr;
using swoc::IPRange;
using swoc::IPSpace;

using swoc::MemSpan;
using swoc::MemArena;

using W = swoc::LocalBufferWriter<256>;

// Property maps for IPSpace.

/// A @c Property is a collection of names which are values of the property.
class Property {
  using self_type = Property;
public:
  /// A handle to an instance.
  using Handle = std::unique_ptr<self_type>;

  /** Construct an instance.
   *
   * @param idx THe property index in a row.
   */
  Property(TextView  const& name) : _name(name) {};

  virtual ~Property() = default;

  virtual size_t size() const = 0;

  unsigned idx() const { return _idx; }

  virtual bool needs_localized_token() const { return false; }

  self_type & assign_idx(unsigned idx) { _idx = idx; return *this; }
  /// Assign the @a offset in bytes for this property.
  self_type & assign_offset(size_t offset) { _offset = offset; return *this; }
  size_t offset() const { return _offset; }

  virtual bool parse(TextView token, MemSpan<std::byte> span) = 0;

protected:
  TextView _name;
  unsigned _idx = std::numeric_limits<unsigned>::max(); ///< Column index.
  size_t _offset = std::numeric_limits<size_t>::max(); ///< Offset into a row.
};

// ---

class Table {
  using self_type = Table;
public:
  static constexpr char SEP = ',';

  Table() = default;

  /** Add a column to the table.
   *
   * @param col Column descriptor.
   * @return @a this
   */
  self_type & add_column(Property::Handle && col);

  /// A row for the table.
  class Row {
    using self_type = Row;
  public:
    Row(MemSpan<std::byte> span) : _data(span) {}
    MemSpan<std::byte> span_for(Property const& prop) const {
      return MemSpan<std::byte>{_data}.remove_prefix(prop.offset());
    }
  protected:
    MemSpan<std::byte> _data;
  };

  bool parse(TextView src);

  Row* find(IPAddr const& addr);

  size_t size() const { return _space.count(); }

  /// Return the property for the column @a idx.
  Property * column(unsigned idx) { return _columns[idx].get(); }

protected:
  size_t _size = 0;
  std::vector<Property::Handle> _columns;

  using space = IPSpace<Row>;
  space _space;

  MemArena _arena;

  TextView token(TextView & src);

  TextView localize(TextView const& src);
};

auto Table::add_column(Property::Handle &&col) -> self_type & {
  col->assign_offset(_size);
  col->assign_idx(_columns.size());
  _size += col->size();
  _columns.emplace_back(std::move(col));
  return *this;
}

TextView Table::localize(TextView const&src) {
  auto span = _arena.alloc(src.size()).rebind<char>();
  memcpy(span, src);
  return span.view();
}

TextView Table::token(TextView & src) {
  TextView::size_type idx = 0;
  // Characters of interest in a null terminated string.
  char sep_list[3] = {'"', SEP, 0};
  bool in_quote_p  = false;
  while (idx < src.size()) {
    // Next character of interest.
    idx = src.find_first_of(sep_list, idx);
    if (TextView::npos == idx) {
      // no more, consume all of @a src.
      break;
    } else if ('"' == src[idx]) {
      // quote, skip it and flip the quote state.
      in_quote_p = !in_quote_p;
      ++idx;
    } else if (SEP == src[idx]) { // separator.
      if (in_quote_p) {
        // quoted separator, skip and continue.
        ++idx;
      } else {
        // found token, finish up.
        break;
      }
    }
  }

  // clip the token from @a src and trim whitespace, quotes
  auto zret = src.take_prefix(idx).trim_if(&isspace).trim('"');
  return zret;
}

bool Table::parse(TextView src) {
  unsigned line_no = 0;
  while (src) {
    auto line = src.take_prefix_at('\n');
    ++line_no;
    auto range_token = line.take_prefix_at(',');
    IPRange range{range_token};
    if (range.empty()) {
      std::cout << W().print("{} is not a valid range specification.", range_token);
      continue; // This is an error, real code should report it.
    }
    MemSpan<std::byte> span = _arena.alloc(_size).rebind<std::byte>();
    Row row{span};
    for ( auto const& col : _columns) {
      auto token = this->token(line);
      if (col->needs_localized_token()) {
        token = this->localize(token);
      }
      if (! col->parse(token, MemSpan<std::byte>{span.data(), col->size()})) {
        std::cout << W().print("Value \"{}\" at index {} on line {} is invalid.", token, col->idx(), line_no);
      }
      span.remove_prefix(col->size());
    }
    _space.mark(range, std::move(row));
  }
  return true;
}

auto Table::find(IPAddr const &addr) -> Row * {
  return _space.find(addr);
}

bool operator == (Table::Row const&, Table::Row const&) { return false; }

// ---

class FlagProperty : public Property {
  using self_type = FlagProperty;
  using super_type = Property;
public:
  static constexpr size_t SIZE = sizeof(bool);
protected:
  size_t size() const override { return SIZE; }
  bool parse(TextView token, MemSpan<std::byte> span) override;
};

class FlagGroupProperty : public Property {
  using self_type = FlagGroupProperty;
  using super_type = Property;
public:
  static constexpr size_t SIZE = sizeof(uint8_t);
  FlagGroupProperty(TextView const& name, std::initializer_list<TextView> tags);

  bool is_set(unsigned idx, Table::Row const& row) const;
protected:
  size_t size() const override { return SIZE; }
  bool parse(TextView token, MemSpan<std::byte> span) override;
  std::vector<TextView> _tags;
};

class TagProperty : public Property {
  using self_type = TagProperty;
  using super_type = Property;
public: // owner
  static constexpr size_t SIZE = sizeof(uint8_t);
  using super_type::super_type;
protected:
  std::vector<TextView> _tags;

  size_t size() const override { return SIZE; }
  bool parse(TextView token, MemSpan<std::byte> span) override;
};

class StringProperty : public Property {
  using self_type = StringProperty;
  using super_type = Property;
public:
  static constexpr size_t SIZE = sizeof(TextView);
  using super_type::super_type;

protected:
  size_t size() const override { return SIZE; }
  bool parse(TextView token, MemSpan<std::byte> span) override;
  bool needs_localized_token() const { return true; }
};

// ---
bool StringProperty::parse(TextView token, MemSpan<std::byte> span) {
  memcpy(span.data(), &token, sizeof(token));
  return true;
}

FlagGroupProperty::FlagGroupProperty(TextView const& name, std::initializer_list<TextView> tags)
    : super_type(name)
{
  _tags.reserve(tags.size());
  for ( auto const& tag : tags ) {
    _tags.emplace_back(tag);
  }
}

bool FlagGroupProperty::parse(TextView token, MemSpan<std::byte> span) {
  if ("-"_tv == token) { return true; } // marker for no flags.
  memset(span, 0);
  while (token) {
    auto tag = token.take_prefix_at(';');
    unsigned j = 0;
    for ( auto const& key : _tags ) {
      if (0 == strcasecmp(key, tag)) {
        span[j/8] |= (std::byte{1} << (j % 8));
        break;
      }
      ++j;
    }
    if (j > _tags.size()) {
      std::cout << W().print("Tag \"{}\" is not recognized.", tag);
      return false;
    }
  }
  return true;
}

bool FlagGroupProperty::is_set(unsigned flag_idx, Table::Row const& row) const {
  auto sp = row.span_for(*this);
  return std::byte{0} != ((sp[flag_idx/8] >> (flag_idx%8)) & std::byte{1});
}

bool TagProperty::parse(TextView token, MemSpan<std::byte> span) {
  // Already got one?
  auto spot = std::find_if(_tags.begin(), _tags.end(), [&](TextView const& tag) { return 0 == strcasecmp(token, tag); });
  if (spot == _tags.end()) { // nope, add it to the list.
    _tags.push_back(token);
    spot = std::prev(_tags.end());
  }
  span.rebind<uint8_t>()[0] = spot - _tags.begin();
  return true;
}

// ---

TEST_CASE("IPSpace properties", "[libswoc][ip][ex][properties]") {
  Table table;
  auto flag_names = { "prod"_tv, "dmz"_tv, "internal"_tv};
  table.add_column(std::make_unique<TagProperty>("owner"));
  table.add_column(std::make_unique<TagProperty>("colo"));
  table.add_column(std::make_unique<FlagGroupProperty>("flags"_tv, flag_names));
  table.add_column(std::make_unique<StringProperty>("Description"));

  TextView src = R"(10.1.1.0/24,asf,cmi,prod;internal,"ASF core net"
192.168.28.0/25,asf,ind,prod,"Indy Net"
192.168.28.128/25,asf,abq,dmz;internal,"Albuquerque zone"
)";

  REQUIRE(true == table.parse(src));
  REQUIRE(3 == table.size());
  auto row = table.find(IPAddr{"10.1.1.56"});
  REQUIRE(nullptr != row);
  CHECK(true == static_cast<FlagGroupProperty*>(table.column(2))->is_set(0, *row));
  CHECK(false == static_cast<FlagGroupProperty*>(table.column(2))->is_set(1, *row));
  CHECK(true == static_cast<FlagGroupProperty*>(table.column(2))->is_set(2, *row));
};

