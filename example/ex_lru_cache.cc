// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Verizon Media

/** @file

    Example of building a thread safe LRU cache using intrusive containers.
*/

#include <unistd.h>
#include <chrono>
#include <utility>
#include <thread>

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
  size_t count() const { return _table.count(); }

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

    Item(self_type && that) : _key(that._key), _value(std::move(that._value)) {}
    Item(K const& key, V && value) : _key(key), _value(std::move(value)) {}

    self_type & operator = (self_type && that) { _key = that._key; _value = std::move(that._value); return *this; }
  };

  struct Linkage {
    static Item * & next_ptr(Item * item) { return item->_list._next; }
    static Item * & prev_ptr(Item * item) { return item->_list._prev; }
  };
  using List = swoc::IntrusiveDList<Linkage>;

  struct Hashing {
    static Item * & next_ptr(Item * item) { return item->_map._next; }
    static Item * & prev_ptr(Item * item) { return item->_map._prev; }
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
      Item * item = _free.take_head();
      if (item != nullptr) {
        new (item) Item(key, std::move(value));
      } else {
        item = _arena.make<Item>(key, std::move(value));
      }
      _table.insert(item);
      _list.append(item);
      if (_list.count() > _max) {
        target = _list.take_head();
        _table.erase(target);
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
  static constexpr size_t N_THREAD = 16; ///< Number of threads.
  static constexpr size_t N_ITER = 20000;

  std::array<std::thread, N_THREAD> threads;
  std::mutex gate_m;
  std::condition_variable gate_cv;
  std::atomic<int> count = -1;

  struct Data {
    time_point _expire;
    int _code = 0;
  };

  LRU<IPAddr, Data> lru;
  lru.insert(swoc::IP4Addr{"172.17.56.93"}, Data{time_point(), 2});

  auto worker = [&] () -> void {
    unsigned dummy;
    {
      std::unique_lock _(gate_m);
      gate_cv.wait(_, [&] () {return count >= 0; });
    }
    swoc::IP4Addr addr((reinterpret_cast<uintptr_t>(&dummy) >> 16) & 0xFFFFFFFF);
    auto tp = time_point();
    for ( unsigned i = 0 ; i < N_ITER ; ++i ) {
      lru.insert(addr, Data{tp, 1});
      addr = htonl(addr.host_order() + 1);
    }
    {
      std::unique_lock _(gate_m);
      ++count;
    }
    gate_cv.notify_all();
  };

  for ( unsigned i = 0 ; i < N_THREAD; ++i ) {
    threads[i] = std::thread(worker);
  }

  auto t0 = std::chrono::system_clock::now();
  {
    std::unique_lock _(gate_m);
    count = 0;
    gate_cv.notify_all();
  }

  {
    std::unique_lock _(gate_m);
    gate_cv.wait(_, [&] () { return count == N_THREAD; });
  }
  auto tf = std::chrono::system_clock::now();
  auto delta = tf - t0;

  std::cout << "Done in " << delta.count() << " with " << lru.count() << std::endl;
  std::cout << "ns per operation " << (delta.count() / (N_THREAD * N_ITER)) << std::endl;
  for ( unsigned i = 0 ; i < N_THREAD ; ++i ) {
    threads[i].join();
  }

  return 0;
}
