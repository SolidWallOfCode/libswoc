#pragma once

#include <array>
#include <vector>
#include <iterator>
#include <cstddef>

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
protected:
  /// Raw storage for an instance.
  union TBlock {
    struct {} _nil; ///< Target for default constructor.
    T _t; ///< aliased instance.

    /// Default constructor, required to make this default constructable for @c std::array.
    /// @internal By design, this does nothing. It a goal to not construct a @a T instance.
    TBlock() {}
  };

  using StaticStore = std::array<TBlock, N>; ///< Static (instance local) storage.
  using DynamicStore = std::vector<T>; ///< Dynamic (heap) storage.

public:
  /// STL compliance types.
  using value_type = T;
  using allocator_type = A;
  using size_type = typename vector_type::size_type;
  using difference_type = typename vector_type::difference_type;

  /// Default constructor, construct an empty container.
  Vectray();

  /** Index operator.
   *
   * @param idx Index of element.
   * @return A reference to the element.
   */
  T & operator [] (size_type idx);

  /** Index operator (const).
   *
   * @param idx Index of element.
   * @return A @c const reference to the element.
   */
  T const& operator [] (size_type idx) const;

  /** Add an element to the end of the current elements.
   *
   * @param src Element to add.
   * @return @a this.
   */
  self_type & push_back(T const& src);

  /** Remove an element from the end of the current elements.
   *
   * @return @a this.
   */
  self_type & pop_back();

   /// @return The number of elements in the container.
  size_type size() const;

  // iteration

  /// Constant iteration.
  class const_iterator {
    using self_type = const_iterator; ///< Self reference type.
    friend class Vectray; ///< Allow container access to internals.

  public:
    // STL compliance.
    using value_type = const typename Vectray::value_type; /// Import for API compliance.
    using iterator_category = std::bidirectional_iterator_tag;
    using pointer           = value_type *;
    using reference         = value_type &;
    using difference_type   = typename Vectray::difference_type;

    /// Default constructor.
    const_iterator();

    /// Pre-increment.
    /// Move to the next element in the list.
    /// @return The iterator.
    self_type &operator++();

    /// Pre-decrement.
    /// Move to the previous element in the list.
    /// @return The iterator.
    self_type &operator--();

    /// Post-increment.
    /// Move to the next element in the list.
    /// @return The iterator value before the increment.
    self_type operator++(int);

    /// Post-decrement.
    /// Move to the previous element in the list.
    /// @return The iterator value before the decrement.
    self_type operator--(int);

    /// Dereference.
    /// @return A reference to the referent.
    value_type &operator*() const;

    /// Dereference.
    /// @return A pointer to the referent.
    value_type *operator->() const;

    /// Equality
    bool operator==(self_type const &that) const;

    /// Inequality
    bool operator!=(self_type const &that) const;

  protected:
    // Stored non-const to make implementing @c iterator easier. This class provides the required @c
    // const protection.

    bool _static_p = true; ///< Is the iterator a static storage iterator?

    // Use anonymous union to promote these into the enclosing class scope.
    union {
      typename StaticStore::iterator _static; ///< Iteration over static elements.
      typename DynamicStore::iterator _dynamic; ///< Iteration over dynamic elements.
    };

    /// Internal constructor for containers.
    const_iterator(typename StaticStore::iterator const& spot) : _static_p(true), _static(spot) {}
    /// Internal constructor for containers.
    const_iterator(typename DynamicStore::iterator const& spot) : _static_p(false), _dynamic(spot) {}
  };

  /// Iterator for the list.
  class iterator : public const_iterator {
    using self_type  = iterator;       ///< Self reference type.
    using super_type = const_iterator; ///< Super class type.

    friend class Vectray;

  public:
    using list_type  = Vectray;                 /// Must hoist this for direct use.
    using value_type = typename list_type::value_type; /// Import for API compliance.
    // STL algorithm compliance.
    using iterator_category = std::bidirectional_iterator_tag;
    using pointer           = value_type *;
    using reference         = value_type &;

    /// Default constructor.
    iterator() = default;

    /// Pre-increment.
    /// Move to the next element in the list.
    /// @return The iterator.
    self_type &operator++();

    /// Pre-decrement.
    /// Move to the previous element in the list.
    /// @return The iterator.
    self_type &operator--();

    /// Post-increment.
    /// Move to the next element in the list.
    /// @return The iterator value before the increment.
    self_type operator++(int);

    /// Post-decrement.
    /// Move to the previous element in the list.
    /// @return The iterator value before the decrement.
    self_type operator--(int);

    /// Dereference.
    /// @return A reference to the referent.
    value_type &operator*() const;

    /// Dereference.
    /// @return A pointer to the referent.
    value_type *operator->() const;

    /// Convenience conversion to pointer type
    /// Because of how this list is normally used, being able to pass an iterator as a pointer is quite convenient.
    /// If the iterator isn't valid, it converts to @c nullptr.
    operator value_type *() const;

  protected:
    /// Internal constructor for containers.
    iterator(typename StaticStore::iterator const& spot) : super_type(spot) {}
    /// Internal constructor for containers.
    iterator(typename DynamicStore::iterator const& spot) : super_type(spot) {}
  };

  const_iterator begin() const;
  const_iterator end() const;
  iterator begin();
  iterator end();

protected:
  /// Number of valid static elements.
  /// This is set to a negative value if storage is switched over to dynamic storage.
  ssize_t _count = 0;

  StaticStore _static; ///< Static storage.
  DynamicStore _vector; ///< Dynamic storage.
};

// --- Implementation ---

template<typename T, size_t N, typename A>
Vectray<T,N,A>::Vectray() {}

template<typename T, size_t N, typename A>
T&Vectray<T,N,A>::operator[](size_type idx) {
  return _count < 0 ? _vector[idx] : _static[idx]._t;
}

template<typename T, size_t N, typename A>
T const&Vectray<T,N,A>::operator[](size_type idx) const {
  return _count < 0 ? _vector[idx] : _static[idx];
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::push_back(const T&src) -> self_type& {
  if (_count < 0) { // once you go vector, you never go back.
    _vector.push_back(src);
  } else if (_count >= N) {
    _vector.reserve(N + 1); // need at least this many.
    // Copy over, destructing as we go.
    for ( size_type idx = 0 ; idx < _count ; ++idx ) {
      T& t = (_static[idx]._t);
      _vector.push_back(t);
      t.~T(); // destroy original.
    }
    _vector.push_back(src); // plus the actually requested element.
    _count = -1; /// Mark switch to dynamic storage.
  } else { // still in the static area.
    new (&_static[_count++]) T(src);
  }
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::pop_back() -> self_type& {
  if (_count < 0) {
    _vector.pop_back();
  } else {
    --_count;
    _static[_count]._t.~T();
  }
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::size() const -> size_type {
  return _count < 0 ? _vector.size() : _count;
}

template<typename T, size_t N, typename A>
auto  Vectray<T,N,A>::begin() const -> const_iterator {
  auto nc_this = const_cast<self_type*>(this);
  return _count < 0 ? const_iterator{ nc_this->_vector.begin() } : const_iterator{ nc_this->_static.begin()}; }

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::end() const -> const_iterator {
  auto nc_this = const_cast<self_type*>(this);
  return _count < 0 ? const_iterator{ nc_this->_vector.end() } : const_iterator{ nc_this->_static.end() };
}

template<typename T, size_t N, typename A>
auto  Vectray<T,N,A>::begin() -> iterator {
  return _count < 0 ? iterator{ this->_vector.begin() } : iterator{ this->_static.begin()}; }

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::end() -> iterator {
  return _count < 0 ? iterator{ this->_vector.end() } : iterator{ this->_static.end() };
}
// --- iterators

template<typename T, size_t N, typename A>
Vectray<T,N,A>::const_iterator::const_iterator() : _static() {
}

template < typename T, size_t N, typename A>
bool Vectray<T,N,A>::const_iterator::operator==(self_type const &that) const {
  return _static_p
         ? (true == that._static_p && _static == that._static)
         : (false == that._static_p && _dynamic == that._dynamic)
      ;
}

template<typename T, size_t N, typename A>
bool Vectray<T,N,A>::const_iterator::operator!=(
    Vectray::const_iterator::self_type const&that) const {
  return ! (*this == that);
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator++() -> self_type & {
  if (_static_p) {
    ++_static;
  } else {
    ++_dynamic;
  }
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator--() -> self_type & {
  if (_static_p) {
    ++_static;
  } else {
    ++_dynamic;
  }
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator++(int) -> self_type {
  self_type zret { *this };
  ++*this;
  return zret;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator--(int) -> self_type {
  self_type zret { *this };
  --*this;
  return zret;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator*() const -> value_type & {
  if (_static_p) { return _static->_t; }
  return *_dynamic;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::const_iterator::operator->() const -> value_type * {
  return _static_p ? &(_static->_t) : _dynamic.operator ->();
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator++() -> self_type & {
  this->super_type::operator++();
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator++(int) -> self_type {
  self_type zret { *this };
  ++*this;
  return zret;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator--() -> self_type & {
  this->super_type::operator--();
  return *this;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator--(int) -> self_type {
  self_type zret { *this };
  --*this;
  return zret;
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator*() const -> value_type & {
  if (this->_static_p) { return this->_static->_t; }
  return *(this->_dynamic);
}

template<typename T, size_t N, typename A>
auto Vectray<T,N,A>::iterator::operator->() const -> value_type * {
  return this->_static_p ? &(this->_static->_t) : this->_dynamic.operator ->();
}

}} // namespace swoc

