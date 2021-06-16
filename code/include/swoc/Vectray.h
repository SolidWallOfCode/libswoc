#pragma once

#include <array>
#include <vector>
#include <variant>
#include <new>
#include <cstddef>

#include <swoc/MemSpan.h>
#include <swoc/swoc_meta.h>

namespace swoc { inline namespace SWOC_VERSION_NS {

/** Vectray provides a combination of static and dynamic storage modeled as an array.
 *
 * @tparam T Type of elements in the array.
 * @tparam N Number of statically allocated elements.
 * @tparam A Allocator.
 *
 * The goal is to provide static storage for the common case, avoiding memory allocation, while
 * still handling exceptional cases that need more storage. A common case is for @a N == 1 where
 * there is almost always a single value, but it is possible to have multiple values. @c Vectray
 * makes the single value case require no allocation while transparently handling the multiple
 * value case.
 *
 * The interface is designed to mimic that of @c std::vector.
 */
template < typename T, size_t N, class A = std::allocator<T> >
class Vectray {
  using self_type = Vectray; ///< Self reference type.
  using vector_type = std::vector<T, A>;

public: // STL compliance types.
  using value_type = T;
  using allocator_type = A;
  using size_type = typename vector_type::size_type;
  using difference_type = typename vector_type::difference_type;
  using iterator = typename swoc::MemSpan<T>::iterator;
  using const_iterator = typename swoc::MemSpan<const T>::iterator;

protected:
  /// Internal (fixed) storage.
  struct FixedStore {
    size_t _count = 0; ///< Number of valid elements.
    allocator_type _a; ///< Allocator instance - used for construction.
    std::array<std::byte, sizeof(T) * N> _raw; ///< Raw memory for element storage.

    FixedStore() = default;
    explicit FixedStore(allocator_type const& a) : _a(a) {}
  };
  using DynamicStore = vector_type; ///< Dynamic (heap) storage.

  /// Generic form for referencing stored objects.
  using span = swoc::MemSpan<T>;
  using const_span = swoc::MemSpan<T const>;

public:
  /// Default constructor, construct an empty container.
  Vectray();

  /// Construct empty instance with allocator.
  constexpr explicit Vectray(allocator_type const& a) : _store(std::in_place_type_t<FixedStore>{}, a) {}

  /** Construct with @a n default constructed elements.
   *
   * @param n Number of elements.
   * @param alloc Allocator (optional - default constructed if not a parameter).
   */
  explicit Vectray(size_type n, allocator_type const& alloc = allocator_type{});

  Vectray(self_type && that) {
    // If @a that is already a vector, always move that here.
    if (DYNAMIC == that._store.index()) {
      _store = std::move(std::get<DYNAMIC>(that._store));
    } else {
      auto & fixed = std::get<FIXED>(_store);
    }
  }

  Vectray(self_type && that, allocator_type const& a);

  /// @return The number of elements in the container.
  size_type size() const;

  /** Index operator.
   *
   * @param idx Index of element.
   * @return A reference to the element.
   */
  T& operator[](size_type idx);

  /** Index operator (const).
   *
   * @param idx Index of element.
   * @return A @c const reference to the element.
   */
  T const& operator[](size_type idx) const;

  /** Append an element by copy.
   *
   * @param src Element to add.
   * @return @a this.
   */
  self_type& push_back(T const& t);

  /** Append an element by direct construction.
   *
   * @tparam Args Constructor parameter types.
   * @param args Constructor arguments.
   * @return @a this
   */
  template < typename ... Args> self_type& emplace_back(Args... args);

  /** Remove an element from the end of the current elements.
   *
   * @return @a this.
   */
  self_type& pop_back();

  /// Iterator for first element.
  const_iterator begin() const;

  /// Iterator past last element.
  const_iterator end() const;

  /// Iterator for last element.
  iterator begin();

  /// Iterator past last element.
  iterator end();

  void reserve(size_type n);

protected:
  /// Content storage.
  /// @note This is constructed as fixed but can change to dynamic. It can never change back.
  std::variant<FixedStore, DynamicStore> _store;

  static constexpr auto FIXED = 0; ///< Variant index for fixed storage.
  static constexpr auto DYNAMIC = 1; ///< Variant index for dynamic storage.

  /// Get the span of the valid items.
  span items();
  /// Get the span of the valid items.
  const_span items() const;

  /// Default size to reserve in the vector when switching to dynamic.
  static constexpr size_type BASE_DYNAMIC_SIZE = (7 * N) / 5;


  /** Transfer from fixed storage to dynamic storage.
   *
   * @param rN Numer of elements of storage to reserve in the vector.
   *
   * @note Must be called at most once for any instance.
   */
  void transfer(size_type rN = BASE_DYNAMIC_SIZE);
};

// --- Implementation ---

template<typename T, size_t N, typename A>
Vectray<T,N,A>::Vectray() {}

template<typename T, size_t N, class A>
Vectray<T, N, A>::Vectray(Vectray::size_type n, allocator_type const& alloc) : Vectray() {
  this->reserve(n);
  while (n-- > 0) {
    this->emplace_back();
  }
}

template<typename T, size_t N, typename A>
T& Vectray<T,N,A>::operator[](size_type idx) {
  return this->items()[idx];
}

template<typename T, size_t N, typename A>
T const& Vectray<T,N,A>::operator[](size_type idx) const {
  return this->items[idx];
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::push_back(const T& t) -> self_type& {
  std::visit(swoc::meta::vary{
      [&](FixedStore& fs) -> void {
        if (fs._count < N) {
          new(reinterpret_cast<T*>(fs._raw.data()) + fs._count++) T(t); // A::traits ?
        } else {
          this->transfer();
          std::get<DYNAMIC>(_store).push_back(t);
        }
      }
      , [&](DynamicStore& ds) -> void {
        ds.push_back(t);
      }
  }, _store);
  return *this;
}

template<typename T, size_t N, class A>
template<typename... Args>
auto Vectray<T, N, A>::emplace_back(Args... args) -> self_type& {
  if (_store.index() == FIXED) {
    auto fs{std::get<FIXED>(_store)};
    if (fs._count < N) {
      new(reinterpret_cast<T*>(fs._raw.data()) + fs._count++) T(std::forward<Args...>(args...)); // A::traits ?
      return *this;
    }
    this->transfer();
  }
  std::get<DYNAMIC>(_store).emplace_back(std::forward<Args...>(args...));
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::size() const -> size_type {
  return std::visit(swoc::meta::vary{
      [](FixedStore const& fs) { return fs._count; }
      , [](DynamicStore const& ds) { return ds.size(); }
  }, _store);
}

// --- iterators
template<typename T, size_t N, typename A>
auto  Vectray<T,N,A>::begin() const -> const_iterator { return this->items().begin(); }

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::end() const -> const_iterator { return this->items().end(); }

template<typename T, size_t N, typename A>
auto  Vectray<T,N,A>::begin() -> iterator { return this->items().begin(); }

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::end() -> iterator { return this->items().end(); }
// --- iterators

template<typename T, size_t N, class A>
void Vectray<T, N, A>::transfer(size_type rN) {
  DynamicStore tmp{std::get<FIXED>(_store)._a};
  tmp.reserve(rN);

  for (auto&& item : this->items()) {
    tmp.emplace_back(item); // move if supported, copy if not.
    item.~T(); // destroy original.
  }
  _store = std::move(tmp);
}

template<typename T, size_t N, class A>
auto Vectray<T, N, A>::items() const -> const_span {
  return std::visit(swoc::meta::vary{
      [](FixedStore const& fs) { return const_span(reinterpret_cast<T const *>(fs._raw.data()), fs._count); }
      , [](DynamicStore const& ds) { return const_span(ds.data(), ds.size()); }
  }, _store);
}

template<typename T, size_t N, class A>
auto Vectray<T, N, A>::items() -> span {
  return std::visit(swoc::meta::vary{
      [](FixedStore & fs) { return span(reinterpret_cast<T *>(fs._raw.data()), fs._count); }
      , [](DynamicStore & ds) { return span(ds.data(), ds.size()); }
  }, _store);
}

template<typename T, size_t N, class A>
void Vectray<T, N, A>::reserve(Vectray::size_type n) {
  if (DYNAMIC == _store.index()) {
    std::get<DYNAMIC>(_store).reserve(n);
  } else if (n > N) {
    this->transfer(n);
  }
}

}} // namespace swoc

