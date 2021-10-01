// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Verizon Media

/** @file

    Example of building a thread safe LRU cache using intrusive containers.
*/

#include <unistd.h>
#include <chrono>
#include <utility>

#include "swoc/TextView.h"
#include "swoc/swoc_ip.h"
#include "swoc/bwf_ip.h"
#include "swoc/bwf_ex.h"
#include "swoc/bwf_std.h"
#include "swoc/swoc_file.h"
#include "swoc/Errata.h"
#include "swoc/IntrusiveDList.h"
#include "swoc/IntrusiveHashMap.h"
#include "swoc/MemArena.h"

using namespace std::literals;
using namespace swoc::literals;

using swoc::TextView;
using swoc::MemSpan;
using swoc::Errata;
using swoc::IPAddr;

using time_point = std::chrono::time_point<std::chrono::system_clock>;

template < typename K, typename V > class LRU {
  using self_type = LRU;
public:
  LRU() = default;

  self_type & insert(K const& key, V && value);
  self_type & erase(K const& key);
  V retrieve(K const& key) const;

protected:
  struct Item {
    using self_type = Item;
    struct Links {
      self_type * _next = nullptr;
      self_type * _prev = nullptr;
    };

    K _key;
    V _value;

    Links _list;
    Links _map;
  };

  struct Linkage {
    static Item * next_ptr(Item * item) { return item->_list._next; }
    static Item * prev_ptr(Item * item) { return item->_list._prev; }
  };
  using List = swoc::IntrusiveDList<Linkage>;

  struct Hashing {
    static Item * next_ptr(Item * item) { return item->_map._next; }
    static Item * prev_ptr(Item * item) { return item->_map._prev; }
    static inline const std::hash<K> Hasher;
    static K const& key_of(Item * item) { return item->_key; }
    static decltype(Hasher(std::declval<K>())) hash_of(K const& key) { return Hasher(key); }
    static bool equal(K const& lhs, K const& rhs) { return lhs == rhs; }
  };
  using Table = swoc::IntrusiveHashMap<Hashing>;

  std::shared_mutex _mutex; ///< Read/write lock.
  size_t _max = 1024; ///< Maximum number of elements.
  List _list; ///< LRU list.
  Table _table; ///< Keyed set of values.
  List _free; ///< Free list.
  swoc::MemArena _arena;
};

template <typename K, typename V> auto LRU<K, V>::insert(K const &key, V &&value) -> self_type & {
  Item *target = nullptr;
  {
    std::unique_lock lock(_mutex);
    auto spot = _table.find(key);
    if (spot != _table.end()) {
      spot->_value = std::move(value);
    } else {
      auto *item = _arena.make<Item>(key, std::move(value));
      _table.insert(item);
      _list.append(item);
      if (_list.size() > _max) {
        target = _list.take_head();
      }
    }
  }

  if (target) {
    target->_key.~K();
    target->_value.~V();
    std::unique_lock lock(_mutex);
    _free.append(target);
  }
  return *this;
}

template <typename K, typename V> auto LRU<K, V>::erase(K const &key) -> self_type & {
  std::unique_lock lock(_mutex);
  auto spot = _table.find(key);
  if (spot != _table.end()) {
    Item * item = spot;
    _table.erase(item);
    _list.erase(item);
    _free.append(item);
  }
  return *this;
}

template <typename K, typename V> auto LRU<K, V>::retrieve(K const &key) const -> V {
  std::shared_lock lock(_mutex);
  auto spot = _table.find(key);
  return spot == _table.end() ? V{} : *spot;
}

// --------------------------------------------------

int main(int, char *[]) {
  struct Data {
    time_point _expire;
    int _code = 0;
  };

  LRU<IPAddr, Data> lru;

  return 0;
}
