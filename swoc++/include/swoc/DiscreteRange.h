#pragma once
// SPDX-License-Identifier: Apache-2.0
// Copyright 2014 Network Geographics

/** @file
    Support classes for creating intervals of numeric values.

    The template class can be used directly via a @c typedef or
    used as a base class if additional functionality is required.
 */

#include <limits>
#include <functional>

#include <swoc/swoc_meta.h>
#include <swoc/RBTree.h>
#include <swoc/MemArena.h>

namespace swoc
{
/// Internal implementation namespace.
namespace detail
{

  /// A set of metafunctions to get extrema from a metric type.
  /// These probe for a static member and falls back to @c std::numeric_limits.
  /// @{
  template <typename M>
  constexpr auto
  maximum(meta::CaseTag<0>) -> M {
    return std::numeric_limits<M>::max();
  }

  template <typename M>
  constexpr auto
  maximum(meta::CaseTag<1>) -> decltype(M::MAX) {
    return M::MAX;
  }

  template <typename M>
  constexpr M
  maximum() {
    return maximum<M>(meta::CaseArg);
  }

  template <typename M>
  constexpr auto
  minimum(meta::CaseTag<0>) -> M {
    return std::numeric_limits<M>::min();
  }

  template <typename M>
  constexpr auto
  minimum(meta::CaseTag<1>) -> decltype(M::MIN) {
    return M::MIN;
  }

  template <typename M>
  constexpr M
  minimum() {
    return minimum<M>(meta::CaseArg);
  }
  /// @}
} // namespace detail

/// Relationship between two intervals.
enum DiscreteRangeRelation {
  NONE,     ///< No common elements.
  EQUAL,    ///< Identical ranges.
  SUBSET,   ///< All elements in LHS are also in RHS.
  SUPERSET, ///< Every element in RHS is in LHS.
  OVERLAP,  ///< There exists at least one element in both LHS and RHS.
  ADJACENT  ///< The two intervals are adjacent and disjoint.
};

/** A range over a discrete finite value metric.
   @tparam T The type for the range values.

   The template argument @a T is presumed to
   - be completely ordered.
   - have prefix increment and decrement operators
   - equality operator
   - have value semantics
   - have minimum and maximum values either
     - members @c MIN and @c MAX that define static instances
     - support @c std::numeric_limits<T>

   The interval is always an inclusive (closed) contiguous interval,
   defined by the minimum and maximum values contained in the interval.
   An interval can be @em empty and contain no values. This is the state
   of a default constructed interval.
 */
template <typename T> class DiscreteRange {
  using self_type   = DiscreteRange;

protected:
  T _min; ///< The minimum value in the interval
  T _max; ///< the maximum value in the interval

public:
  using metric_type = T;
  using Relation = DiscreteRangeRelation;

  static constexpr self_type ALL{detail::minimum<metric_type>(), detail::maximum<metric_type>()};

  /** Default constructor.
      An empty range is constructed.
   */
  constexpr DiscreteRange() : _min(detail::maximum<T>()), _max(detail::minimum<T>()) {}

  /** Construct a singleton range.
   * @param value Single value to be contained by the interval.
   *
   * @note Not marked @c explicit and so serves as a conversion from scalar values to an interval.
   */
  constexpr DiscreteRange(T const &value) : _min(value), _max(value){};

  /** Constructor.
   *
   * @param min Minimum value in the interval.
   * @param max Maximum value in the interval.
   */
  constexpr DiscreteRange(T const &min, T const &max) : _min(min), _max(max) {}

  ~DiscreteRange() = default;

  self_type &assign(metric_type const &min, metric_type const &max);

  /// Set the interval to be a singleton.
  self_type &assign(metric_type const &singleton);

  self_type &assign_min(metric_type const &min);

  self_type &assign_max(metric_type const &max);

  /** Decrement the maximum value.
   *
   * @return @a this.
   */
  self_type &clip_max() { --_max; return *this; }

  /** Get the minimum value in the interval.
      @note The return value is unspecified if the interval is empty.
   */
  metric_type const &min() const;

  /** Get the maximum value in the interval.
      @note The return value is unspecified if the interval is empty.
   */
  metric_type const &max() const;

  bool contains(metric_type const& n) {
    return _min <= n && n <= _max;
  }

  /** Logical intersection test for two intervals.
      @return @c true if there is at least one common value in the
      two intervals, @c false otherwise.
  */
  bool has_intersection_with(self_type const &that) const;

  /** Compute the intersection of two intervals
      @return The interval consisting of values that are contained by
      both intervals. This may be the empty interval if the intervals
      are disjoint.
      @internal Co-variant
   */
  self_type intersection(self_type const &that) const;

  /** Test for adjacency.
      @return @c true if the intervals are adjacent.
      @note Only disjoint intervals can be adjacent.
   */
  bool is_adjacent_to(self_type const &that) const;

  //! Test if the union of two intervals is also an interval.
  bool has_union(self_type const &that) const;

  /** Test if an interval is a superset of or equal to another.
      @return @c true if every value in @c that is also in @c this.
   */
  bool is_superset_of(self_type const &that) const;

  /** Test if an interval is a subset or equal to another.
      @return @c true if every value in @c this is also in @c that.
   */
  bool is_subset_of(self_type const &that) const;

  /** Test if an interval is a strict superset of another.
      @return @c true if @c this is strictly a superset of @a rhs.
   */
  bool is_strict_superset_of(self_type const &that) const;

  /** Test if an interval is a strict subset of another.
      @return @c true if @c this is strictly a subset of @a that.
   */
  bool is_strict_subset_of(self_type const &that) const;

  /** Determine the relationship between @c this and @a that interval.
      @return The relationship type.
   */
  Relation relationship(self_type const &that) const;

  /** Compute the convex hull of this interval and another one.
      @return The smallest interval that is a superset of @c this
      and @a that interval.
      @internal Co-variant
   */
  self_type hull(self_type const &that) const;

  //! Check if the interval is exactly one element.
  bool is_singleton() const;

  //! Check if the interval is empty.
  bool is_empty() const;

  /** Test for empty, operator form.
      @return @c true if the interval is empty, @c false otherwise.
   */
  bool operator!() const { return _min > _max; }

  /** Test for non-empty.
   *
   * @return @c true if there values in the range, @c false if no values in the range.
   */
  explicit operator bool() const { return _min <= _max; }

  /// @return @c true if the range is maximal, @c false otherwise.
  bool is_maximal() const;

  /** Clip interval.
      Remove all element in @c this interval not in @a that interval.
   */
  self_type &operator&=(self_type const &that);

  /** Convex hull.
      Extend interval to cover all elements in @c this and @a that.
   */
  self_type &operator|=(self_type const &that);

  self_type & clear() {
    _min = detail::maximum<T>();
    _max = detail::minimum<T>();
    return *this;
  }
  /** Functor for lexicographic ordering.
      If, for some reason, an interval needs to be put in a container
      that requires a strict weak ordering, the default @c operator @c < will
      not work. Instead, this functor should be used as the comparison
      functor. E.g.
      @code
      typedef std::set<Interval<T>, Interval<T>::lexicographic_order> container;
      @endcode

      @note Lexicographic ordering is a standard tuple ordering where the
      order is determined by pairwise comparing the elements of both tuples.
      The first pair of elements that are not equal determine the ordering
      of the overall tuples.
   */
  struct lexicographic_order : public std::binary_function<self_type, self_type, bool> {
    //! Functor operator.
    bool operator()(self_type const &lhs, self_type const &rhs) const;
  };
};

template <typename T>
bool
DiscreteRange<T>::lexicographic_order::operator()(DiscreteRange::self_type const &lhs, DiscreteRange::self_type const &rhs) const {
  return lhs._min == rhs._min ? lhs._max < rhs._max : lhs._min < rhs._min;
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::assign(metric_type const &min, metric_type const &max) {
  _min = min;
  _max = max;
  return *this;
}

template <typename T>
DiscreteRange<T>
DiscreteRange<T>::hull(DiscreteRange::self_type const &that) const {
  // need to account for invalid ranges.
  return !*this ? that : !that ? *this : self_type(std::min(_min, that._min), std::max(_max, that._max));
}

template <typename T>
typename DiscreteRange<T>::Relation
DiscreteRange<T>::relationship(self_type const &that) const {
  Relation retval = NONE;
  if (this->has_intersection(that)) {
    if (*this == that)
      retval = EQUAL;
    else if (this->is_subset_of(that))
      retval = SUBSET;
    else if (this->is_superset_of(that))
      retval = SUPERSET;
    else
      retval = OVERLAP;
  } else if (this->is_adjacent_to(that)) {
    retval = ADJACENT;
  }
  return retval;
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::assign(metric_type const &singleton) {
  _min = singleton;
  _max = singleton;
  return *this;
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::assign_min(metric_type const &min) {
  _min = min;
  return *this;
}

template <typename T>
bool
DiscreteRange<T>::is_singleton() const {
  return _min == _max;
}

template <typename T>
bool
DiscreteRange<T>::is_empty() const {
  return _min > _max;
}

template <typename T>
bool
DiscreteRange<T>::is_maximal() const {
  return _min == detail::minimum<T>() && _max == detail::maximum<T>();
}

template <typename T>
bool
DiscreteRange<T>::is_strict_superset_of(DiscreteRange::self_type const &that) const {
  return (_min < that._min && that._max <= _max) || (_min <= that._min && that._max < _max);
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::operator|=(DiscreteRange::self_type const &that) {
  if (!*this) {
    *this = that;
  } else if (that) {
    if (that._min < _min) {
      _min = that._min;
    }
    if (that._max > _max) {
      _max = that._max;
    }
  }
  return *this;
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::assign_max(metric_type const &max) {
  _max = max;
  return *this;
}

template <typename T>
bool
DiscreteRange<T>::is_strict_subset_of(DiscreteRange::self_type const &that) const {
  return that.is_strict_superset_of(*this);
}

template <typename T>
bool
DiscreteRange<T>::is_subset_of(DiscreteRange::self_type const &that) const {
  return that.is_superset_of(*this);
}

template <typename T>
T const &
DiscreteRange<T>::min() const {
  return _min;
}

template <typename T>
T const &
DiscreteRange<T>::max() const {
  return _max;
}

template <typename T>
bool
DiscreteRange<T>::has_union(DiscreteRange::self_type const &that) const {
  return this->has_intersection(that) || this->is_adjacent_to(that);
}

template <typename T>
DiscreteRange<T> &
DiscreteRange<T>::operator&=(DiscreteRange::self_type const &that) {
  *this = this->intersection(that);
  return *this;
}

template <typename T>
bool
DiscreteRange<T>::has_intersection_with(DiscreteRange::self_type const &that) const {
  return (that._min <= _min && _min <= that._max) || (_min <= that._min && that._min <= _max);
}

template <typename T>
bool
DiscreteRange<T>::is_superset_of(DiscreteRange::self_type const &that) const {
  return _min <= that._min && that._max <= _max;
}

template <typename T>
bool
DiscreteRange<T>::is_adjacent_to(DiscreteRange::self_type const &that) const {
  /* Need to be careful here. We don't know much about T and we certainly don't know if "t+1"
   * even compiles for T. We do require the increment operator, however, so we can use that on a
   * copy to get the equivalent of t+1 for adjacency testing. We must also handle the possibility
   * T has a modulus and not depend on ++t > t always being true. However, we know that if t1 >
   * t0 then ++t0 > t0.
   */

  if (_max < that._min) {
    T x(_max);
    return ++x == that._min;
  } else if (that._max < _min) {
    T x(that._max);
    return ++x == _min;
  }
  return false;
}

template <typename T>
DiscreteRange<T>
DiscreteRange<T>::intersection(DiscreteRange::self_type const &that) const {
  return {std::max(_min, that._min), std::min(_max, that._max)};
}

/** Equality.
    Two intervals are equal if their min and max values are equal.
    @relates interval
 */
template <typename T>
bool
operator==(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return lhs.min() == rhs.min() && lhs.max() == rhs.max();
}

/** Inequality.
    Two intervals are equal if their min and max values are equal.
    @relates interval
 */
template <typename T>
bool
operator!=(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return !(lhs == rhs);
}

/** Operator form of logical intersection test for two intervals.
    @return @c true if there is at least one common value in the
    two intervals, @c false otherwise.
    @note Yeah, a bit ugly, using an operator that is not standardly
    boolean but
    - There don't seem to be better choices (&&,|| not good)
    - The assymmetry between intersection and union makes for only three natural operators
    - ^ at least looks like "intersects"
    @relates interval
 */
template <typename T>
bool
operator^(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return lhs.has_intersection(rhs);
}

/** Containment ordering.
    @return @c true if @c this is a strict subset of @a rhs.
    @note Equivalent to @c is_strict_subset.
    @relates interval
 */
template <typename T>
inline bool
operator<(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return rhs.is_strict_superset_of(lhs);
}

/** Containment ordering.
    @return @c true if @c this is a subset of @a rhs.
    @note Equivalent to @c is_subset.
    @relates interval
 */
template <typename T>
inline bool
operator<=(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return rhs.is_superset_of(lhs);
}

/** Containment ordering.
    @return @c true if @c this is a strict superset of @a rhs.
    @note Equivalent to @c is_strict_superset.
    @relates interval
 */
template <typename T>
inline bool
operator>(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return lhs.is_strict_superset_of(rhs);
}

/** Containment ordering.
    @return @c true if @c this is a superset of @a rhs.
    @note Equivalent to @c is_superset.
    @relates interval
    */
template <typename T>
inline bool
operator>=(DiscreteRange<T> const &lhs, DiscreteRange<T> const &rhs) {
  return lhs.is_superset_of(rhs);
}

/** A space for a discrete @c METRIC.
 *
 * @tparam METRIC Value type for the space.
 * @tparam PAYLOAD Data stored with values in the space.
 *
 * This is a range based mapping of all values in @c METRIC (the "space") to @c PAYLOAD.
 *
 * @c PAYLOAD is presumed to be relatively cheap to construct and copy.
 *
 * @c METRIC must be
 * - discrete and finite valued type with increment and decrement operations.
 */
template <typename METRIC, typename PAYLOAD> class DiscreteSpace {
  using self_type = DiscreteSpace;

protected:
  using metric_type  = METRIC;  ///< Export.
  using payload_type = PAYLOAD; ///< Export.
  using range_type   = DiscreteRange<METRIC>;

  /// A node in the range tree.
  class Node : public detail::RBNode {
    using self_type  = Node;           ///< Self reference type.
    using super_type = detail::RBNode; ///< Parent class.
    friend class DiscreteSpace;

    PAYLOAD _payload{}; ///< Default constructor, should zero init if @c PAYLOAD is a pointer.
    range_type _range;  ///< Range covered by this node.
    range_type _hull;   ///< Range covered by subtree rooted at this node.

  public:
    /// Linkage for @c IntrusiveDList.
    using Linkage = swoc::IntrusiveLinkageRebind<self_type, super_type::Linkage>;

    Node() = default; ///< Construct empty node.

    /// Construct from @a range and @a payload.
    Node(range_type const &range, PAYLOAD const &payload) : _range(range), _payload(payload) {}
    /// Construct from two metrics and a payload
    Node(METRIC const& min, METRIC const& max, PAYLOAD const& payload) : _range(min, max), _payload(payload) {}

    /// @return The payload in the node.
    PAYLOAD & payload();

    /** Set the @a range of a node.
     *
     * @param range Range to use.
     * @return @a this
     */
    self_type & assign(range_type const &range);

    /** Set the @a payload for @a this node.
     *
     * @param payload Payload to use.
     * @return @a this
     */
    self_type & assign(PAYLOAD const &payload);

    self_type &
    assign_min(METRIC const &m) {
      _range.assign_min(m);
      this->ripple_structure_fixup();
      return *this;
    }

    self_type &
    assign_max(METRIC const &m) {
      _range.assign_max(m);
      this->ripple_structure_fixup();
      return *this;
    }

    /** Decrement the maximum value in the range.
     *
     * The range for the node is reduced by one.
     *
     * @return @a this
     */
    self_type &
    dec_max() {
      _range.dec_max();
      this->ripple_structure_fixup();
      return *this;
    }

    METRIC const&
    min() const {
      return _range.min();
    }
    METRIC const&
    max() const {
      return _range.max();
    }

    void structure_fixup() override;

    self_type * left() { return static_cast<self_type*>(_left); }
    self_type * right() { return static_cast<self_type*>(_right); }

  };

  using Direction = typename Node::Direction;

  Node *_root = nullptr;                        ///< Root node.
  IntrusiveDList<typename Node::Linkage> _list; ///< In order list of nodes.
  swoc::MemArena _arena{4000}; ///< Memory Storage.
  swoc::FixedArena<Node> _fa{_arena}; ///< Node allocator and free list.

  // Utility methods to avoid having casts scattered all over.
  Node *
  prev(Node *n) {
    return Node::Linkage::prev_ptr(n);
  }
  Node *
  next(Node *n) {
    return Node::Linkage::next_ptr(n);
  }
  Node *
  left(Node *n) {
    return static_cast<Node *>(n->_left);
  }
  Node *
  right(Node *n) {
    return static_cast<Node *>(n->_right);
  }

public:
  using iterator = typename decltype(_list)::iterator;

  DiscreteSpace() = default;
  ~DiscreteSpace();

  /** Set the @a payload for a @a range
   *
   * @param range Range to mark.
   * @param payload Payload to set.
   * @return @a this
   *
   * Values in @a range are set to @a payload regardless of the current state.
   */
  self_type &mark(range_type const &range, PAYLOAD const &payload);

  /** Erase a @a range.
   *
   * @param range Range to erase.
   * @return @a this
   *
   * All values in @a range are removed from the space.
   */
  self_type &erase(range_type const &range);

  /** Blend a @a color to a @a range.
   *
   * @tparam F Functor to blend payloads.
   * @tparam U type to blend in to payloads.
   * @param range Range for blending.
   * @param color Payload to blend.
   * @return @a this
   *
   * @a color is blended to values in @a range. If an address in @a range does not have a payload,
   * its payload is set a default constructed @c PAYLOAD blended with @a color. If such an address
   * does have a payload, @a color is blended in to that payload using @blender. The existing color
   * is passed as the first argument and @a color as the second argument. The functor is expected to
   * update the first argument to be the blended color. The function must return a @c bool to
   * indicate whether the blend resulted in a valid color. If @c false is returned, the blended
   * region is removed from the space.
   */
  template < typename F, typename U = PAYLOAD >
  self_type &blend(range_type const &range, U const &color, F && blender);

  /** Fill @a range with @a payload.
   *
   * @param range Range to fill.
   * @param payload Payload to use.
   * @return @a this
   *
   * Values in @a range that do not have a payload are set to @a payload. Values in the space are
   * not changed.
   */
  self_type &fill(range_type const &range, PAYLOAD const &payload);

  /** Find the payload at @a metric.
   *
   * @param metric The metric for which to search.
   * @return The payload for @a metric if found, @c nullptr if not found.
   */
  PAYLOAD * find(METRIC const &metric);

  /// @return The number of distinct ranges.
  size_t count() const;

  iterator begin() { return _list.begin(); }
  iterator end() { return _list.end(); }

  /// Remove all ranges.
  void clear() {
    for (auto &node : _list) {
      std::destroy_at(&node.payload());
    }
    _list.clear();
    _root = nullptr;
    _arena.clear();
    _fa.clear();
  }

protected:
  /** Find the lower bound range for @a target.
   *
   * @param target Lower bound value.
   * @return The rightmost range that starts at or before @a target, or @c nullptr if all ranges start
   * after @a target.
   */
  Node *lower_bound(METRIC const &target);

  /// @return The first node in the tree.
  Node * head();

  /** Insert @a node before @a spot.
   *
   * @param spot Target node.
   * @param node Node to insert.
   */
  void insert_before(Node *spot, Node *node);

  /** Insert @a node after @a spot.
   *
   * @param spot Target node.
   * @param node Node to insert.
   */
  void insert_after(Node *spot, Node *node);

  void prepend(Node *node);

  void append(Node *node);

  void
  remove(Node *node) {
    _root = static_cast<Node *>(node->remove());
    _list.erase(node);
    _fa.destroy(node);
  }
};

// ---

template <typename METRIC, typename PAYLOAD>
PAYLOAD &
DiscreteSpace<METRIC, PAYLOAD>::Node::payload() {
  return _payload;
}

template <typename METRIC, typename PAYLOAD>
auto
DiscreteSpace<METRIC, PAYLOAD>::Node::assign(DiscreteSpace::range_type const &range) -> self_type &{
  _range = range;
  return *this;
}

template <typename METRIC, typename PAYLOAD>
auto
DiscreteSpace<METRIC, PAYLOAD>::Node::assign(PAYLOAD const &payload) -> self_type & {
  _payload = payload;
  return *this;
}

template<typename METRIC, typename PAYLOAD>
void DiscreteSpace<METRIC, PAYLOAD>::Node::structure_fixup() {
  if (_left) {
    if (_right) {
      _hull = this->left()->_range.hull(this->right()->_range);
    } else {
      _hull = this->left()->_range;
    }
  } else if (_right) {
    _hull = this->right()->_range;
  }
}

// ---

template <typename METRIC, typename PAYLOAD>
DiscreteSpace<METRIC, PAYLOAD>::~DiscreteSpace() {
  // Destruct all the payloads - the nodes themselves are in the arena and disappear with it.
  for (auto &node : _list) {
    std::destroy_at(&node.payload());
  }
}

template <typename METRIC, typename PAYLOAD>
size_t DiscreteSpace<METRIC, PAYLOAD>::count() const { return _list.count(); }

template <typename METRIC, typename PAYLOAD>
auto
DiscreteSpace<METRIC, PAYLOAD>::head() -> Node *{
  return static_cast<Node *>(_list.head());
}

template <typename METRIC, typename PAYLOAD>
PAYLOAD *
DiscreteSpace<METRIC, PAYLOAD>::find(METRIC const &metric) {
  auto n = _root; // current node to test.
  while (n) {
    if (metric < n->min()) {
      if (n->_hull.contains(metric)) {
        n = n->left();
      } else {
        return nullptr;
      }
    } else if (n->max() < metric) {
      if (n->_hull.contains(metric)) {
        n = n->right();
      } else {
        return nullptr;
      }
    } else {
      return &n->payload();
    }
  }
  return nullptr;
}

template <typename METRIC, typename PAYLOAD>
auto DiscreteSpace<METRIC, PAYLOAD>::lower_bound(METRIC const &target) -> Node * {
  Node *n    = _root;   // current node to test.
  Node *zret = nullptr; // best node so far.
  while (n) {
    if (target < n->min()) {
      n = left(n);
    } else {
      zret = n; // this is a better candidate.
      if (n->max() < target) {
        n = right(n);
      } else {
        break;
      }
    }
  }
  return zret;
}

template<typename METRIC, typename PAYLOAD>
void DiscreteSpace<METRIC, PAYLOAD>::prepend(DiscreteSpace::Node *node) {
  if (!_root) {
    _root = node;
  } else {
    _root = static_cast<Node *>(_list.head()->set_child(node, Direction::LEFT)->rebalance_after_insert());
  }
  _list.prepend(node);
}

template<typename METRIC, typename PAYLOAD>
void DiscreteSpace<METRIC, PAYLOAD>::append(DiscreteSpace::Node *node) {
  if (!_root) {
    _root = node;
  } else {
    // The last node has no right child, or it wouldn't be the last.
    _root = static_cast<Node *>(_list.tail()->set_child(node, Direction::RIGHT)->rebalance_after_insert());
  }
  _list.append(node);
}

template <typename METRIC, typename PAYLOAD>
void
DiscreteSpace<METRIC, PAYLOAD>::insert_before(DiscreteSpace::Node *spot, DiscreteSpace::Node *node) {
  if (left(spot) == nullptr) {
    spot->set_child(node, Direction::LEFT);
  } else {
    // If there's a left child, there's a previous node, therefore spot->_prev is valid.
    // Further, the previous node must be the rightmost descendant node of the left subtree
    // and therefore has no current right child.
    spot->_prev->set_child(node, Direction::RIGHT);
  }

  _list.insert_before(spot, node);
  _root = static_cast<Node *>(node->rebalance_after_insert());
}

template <typename METRIC, typename PAYLOAD>
void
DiscreteSpace<METRIC, PAYLOAD>::insert_after(DiscreteSpace::Node *spot, DiscreteSpace::Node *node) {
  Node *c = right(spot);
  if (!c) {
    spot->set_child(node, Direction::RIGHT);
  } else {
    // If there's a right child, there's a successor node, and therefore @a _next is valid.
    // Further, the successor node must be the left most descendant of the right subtree
    // therefore it doesn't have a left child.
    spot->_next->set_child(node, Direction::LEFT);
  }

  _list.insert_after(spot, node);
  _root = static_cast<Node *>(node->rebalance_after_insert());
}

template <typename METRIC, typename PAYLOAD>
DiscreteSpace<METRIC, PAYLOAD> &
DiscreteSpace<METRIC, PAYLOAD>::mark(DiscreteSpace::range_type const &range, PAYLOAD const &payload) {
  Node *n = this->lower_bound(range.min()); // current node.
  Node *x = nullptr;                       // New node, gets set if we re-use an existing one.
  Node *y = nullptr;                       // Temporary for removing and advancing.

  METRIC max_plus_1 = range.max();
  ++max_plus_1; // careful to use this only in places there's no chance of overflow.

  /*  We have lots of special cases here primarily to minimize memory
      allocation by re-using an existing node as often as possible.
  */
  if (n) {
    // Watch for wrap.
    METRIC min_minus_1 = range.min();
    --min_minus_1;
    if (n->min() == range.min()) {
      // Could be another span further left which is adjacent.
      // Coalesce if the data is the same. min_minus_1 is OK because
      // if there is a previous range, min is not zero.
      Node *p = prev(n);
      if (p && p->payload() == payload && p->max() == min_minus_1) {
        x = p;
        n = x; // need to back up n because frame of reference moved.
        x->assign_max(range.max());
      } else if (n->max() <= range.max()) {
        // Span will be subsumed by request span so it's available for use.
        x = n;
        x->assign_max(range.max()).assign(payload);
      } else if (n->payload() == payload) {
        return *this; // request is covered by existing span with the same data
      } else {
        // request span is covered by existing span.
        x = new Node{range, payload}; //
        n->assign_min(max_plus_1);    // clip existing.
        this->insert_before(n, x);
        return *this;
      }
    } else if (n->payload() == payload && n->max() >= min_minus_1) {
      // min_minus_1 is safe here because n->_min < min so min is not zero.
      x = n;
      // If the existing span covers the requested span, we're done.
      if (x->max() >= range.max()) {
        return *this;
      }
      x->assign_max(range.max());
    } else if (n->max() <= range.max()) {
      // Can only have left skew overlap, otherwise disjoint.
      // Clip if overlap.
      if (n->max() >= range.min()) {
        n->assign_max(min_minus_1);
      } else if (nullptr != (y = next(n)) && y->max() <= range.max()) {
        // because @a n was selected as the minimum it must be the case that
        // y->min >= min (or y would have been selected). Therefore in this
        // case the request covers the next node therefore it can be reused.
        x = y;
        x->assign(range).assign(payload);
        n = x; // this gets bumped again, which is correct.
      }
    } else {
      // Existing span covers new span but with a different payload.
      // We split it, put the new span in between and we're done.
      // max_plus_1 is valid because n->_max > max.
      Node *r;
      x = _fa.make(range, payload);
      r = _fa.make(range_type{max_plus_1, n->max()}, n->payload());
      n->assign_max(min_minus_1);
      this->insert_after(n, x);
      this->insert_after(x, r);
      return *this; // done.
    }
    n = next(n); // lower bound span handled, move on.
    if (!x) {
      x = _fa.make(range, payload);
      if (n) {
        this->insert_before(n, x);
      } else {
        this->append(x); // note that since n == 0 we'll just return.
      }
    }
  } else if (nullptr != (n = this->head()) &&                  // at least one node in tree.
             n->payload() == payload &&                            // payload matches
             (n->max() <= range.max() || n->min() <= max_plus_1) // overlap or adj.
  ) {
    // Same payload with overlap, re-use.
    x = n;
    n = next(n);
    x->assign_min(range.min());
    if (x->max() < range.max()) {
      x->assign_max(range.max());
    }
  } else {
    x = _fa.make(range, payload);
    this->prepend(x);
  }

  // At this point, @a x has the node for this span and all existing spans of
  // interest start at or past this span.
  while (n) {
    if (n->max() <= range.max()) { // completely covered, drop span, continue
      y = n;
      n = next(n);
      this->remove(y);
    } else if (max_plus_1 < n->min()) { // no overlap, done.
      break;
    } else if (n->payload() == payload) { // skew overlap or adj., same payload
      x->assign_max(n->max());
      y = n;
      n = next(n);
      this->remove(y);
    } else if (n->min() <= range.max()) { // skew overlap different payload
      n->assign_min(max_plus_1);
      break;
    }
  }

  return *this;
}

template <typename METRIC, typename PAYLOAD>
DiscreteSpace<METRIC, PAYLOAD> &
DiscreteSpace<METRIC, PAYLOAD>::fill(DiscreteSpace::range_type const &range, PAYLOAD const &payload) {
  // Rightmost node of interest with n->_min <= min.
  Node *n = this->lower_bound(range.min());
  Node *x = nullptr; // New node (if any).
  // Need copies because we will modify these.
  auto min = range.min();
  auto max = range.max();

  // Handle cases involving a node of interest to the left of the
  // range.
  if (n) {
    if (n->_min < min) {
      auto min_1 = min;
      --min_1;               // dec is OK because min isn't zero.
      if (n->max() < min_1) { // no overlap, not adjacent.
        n = next(n);
      } else if (n->max() >= max) { // incoming range is covered, just discard.
        return *this;
      } else if (n->payload != payload) { // different payload, clip range on left.
        min = n->max();
        ++min;
        n = next(n);
      } else { // skew overlap or adjacent to predecessor with same payload, use node and continue.
        x = n;
        n = next(n);
      }
    }
  } else {
    n = this->head();
  }

  // Work through the rest of the nodes of interest.
  // Invariant: n->min() >= min

  // Careful here -- because max_plus1 might wrap we need to use it only if we can be certain it
  // didn't. This is done by ordering the range tests so that when max_plus1 is used when we know
  // there exists a larger value than max.
  auto max_plus1 = max;
  ++max_plus1;

  /* Notes:
     - max (and thence also max_plus1) never change during the loop.
     - we must have either x != 0 or adjust min but not both for each loop iteration.
  */
  while (n) {
    if (n->data() == payload) {
      if (x) {
        if (n->_max <= max) { // next range is covered, so we can remove and continue.
          this->remove(n);
          n = next(x);
        } else if (n->_min <= max_plus1) {
          // Overlap or adjacent with larger max - absorb and finish.
          x->assign_max(n->_max);
          this->remove(n);
          return *this;
        } else {
          // have the space to finish off the range.
          x->assign_max(max);
          return *this;
        }
      } else {                // not carrying a span.
        if (n->_max <= max) { // next range is covered - use it.
          x = n;
          x->assign_min(min);
          n = next(n);
        } else if (n->_min <= max_plus1) {
          n->assign_min(min);
          return *this;
        } else { // no overlap, space to complete range.
          this->insert_before(n, _fa.make(min, max, payload));
          return *this;
        }
      }
    } else { // different payload
      if (x) {
        if (max < n->_min) { // range ends before n starts, done.
          x->assign_max(max);
          return *this;
        } else if (max <= n->_max) { // range ends before n, done.
          x->assign_max(n->_min).dec_max();
          return *this;
        } else { // n is contained in range, skip over it.
          x->assign_max(n->_min).dec_max();
          x   = nullptr;
          min = n->_max;
          ++min; // OK because n->_max maximal => next is null.
          n = next(n);
        }
      } else {               // no carry node.
        if (max < n->_min) { // entirely before next span.
          this->insert_before(n, new Node(min, max, payload));
          return *this;
        } else {
          if (min < n->_min) { // leading section, need node.
            auto y = _fa.make(min, n->_min, payload);
            y->dec_max();
            this->insert_before(n, y);
          }
          if (max <= n->_max) { // nothing past node
            return *this;
          }
          min = n->_max;
          ++min;
          n = next(n);
        }
      }
    }
  }
  // Invariant: min is larger than any existing range maximum.
  if (x) {
    x->assign_max(max);
  } else {
    this->append(_fa.make(min, max, payload));
  }
  return *this;
}

template<typename METRIC, typename PAYLOAD>
template<typename F, typename U>
auto
DiscreteSpace<METRIC, PAYLOAD>::blend(DiscreteSpace::range_type const&range, U const& color
                                      , F &&blender) -> self_type & {
  // Do a base check for the color to use on unmapped values. If self blending on @a color
  // is @c false, then do not color currently unmapped values.
  auto plain_color = PAYLOAD();
  bool plain_color_p = blender(plain_color, color);

  auto node_cleaner = [&] (Node * ptr) -> void { _fa.destroy(ptr); };
  // Used to hold a temporary blended node - @c release if put in space, otherwise cleaned up.
  using unique_node = std::unique_ptr<Node, decltype(node_cleaner)>;

  // Rightmost node of interest with n->_min <= min.
  Node *n = this->lower_bound(range.min());
  Node *pred = nullptr;

  // This doesn't change, compute outside loop.
  auto range_max_plus_1 = range.max();
  ++range_max_plus_1; // only use in contexts where @a max < METRIC max value.

  // Update every loop to be the place to start blending.
  auto range_min = range.min();

  if (nullptr == n) {
    n = this->head();
  }

  // Process @a n, covering the values from the previous range to @a n.max
  while (n) {
    // min is the smallest value of interest, need to fill from there on to the right.
    metric_type min, min_minus_1;

    pred = n ? prev(n) : nullptr;
    if (pred && pred->max() >= range.min()) {
      min_minus_1 = min = pred->max();
      ++min;
    } else {
      min_minus_1 = min = range.min();
      --min_minus_1;
      if (pred && pred->max() < min_minus_1) {
        pred = nullptr;
      }
    }
    // if @a pred is set, then it's adjacent or overlapping on the left.

    // Do some important computations once, caching the results.
    // @a n extends past @a range, so the trailing segment must be dealt with.
    bool right_ext_p = n->max() > range.max();
    // @a n overlaps with @a range, but only to the right.
    bool right_overlap_p = n->min() <= range.max() && n->min() >= range.min();
    // @a n is adjacent on the right to @a range.
    bool right_adj_p = !right_overlap_p && n->min() == range_max_plus_1;
    // @a n has the same color as would be used for unmapped values.
    bool n_plain_colored_p = plain_color_p && (n->payload() == plain_color);
    // @a pred has the same color as would be used for unmapped values.
    bool pred_plain_colored_p = plain_color_p && pred && pred->payload() == plain_color;

    // Check for no right overlap - that means the next node is past the target range.
    if (!right_overlap_p) {
      if (right_adj_p && n_plain_colored_p) { // can pull @a n left to cover
        n->assign_min(min);
        if (pred_plain_colored_p) { // if that touches @a pred with same color, collapse.
          n->assign_min(pred->min());
          this->remove(pred);
        }
      } else if (pred_plain_colored_p) { // can pull @a pred right to cover.
        pred->assign_max(range.max());
      } else { // Must add new range.
        this->insert_after(n, _fa.make(min, range.max(), plain_color));
      }
      return *this;
    }

    // Invariant: There is overlap between @a n and @a range.

    // Fill from @a min to @a n.min - 1
    if (plain_color_p && min < n->min()) { // can fill and there's space to fill.
      if (n->payload() == plain_color) {
        if (pred && pred->payload() == n->payload()) {
          auto pred_min{pred->min()};
          this->remove(pred);
          n->assign_min(pred_min);
        } else {
          n->assign_min(min);
        }
      } else {
        auto n_min_minus_1{n->min()};
        --n_min_minus_1;
        if (pred && pred->payload() == plain_color) {
          pred->assign_max(n_min_minus_1);
        } else {
          this->insert_before(n, _fa.make(min, n_min_minus_1, plain_color));
        }
      }
    }

    // Create a node with the blend for the overlap and then update / replace @a n as needed.
    auto max { right_ext_p ? range.max() : n->max() }; // smallest boundary of range and @a n.
    unique_node fill { _fa.make(n->min(), max, n->payload()), node_cleaner };
    bool fill_p = blender(fill->payload(), color); // fill or clear?
    auto next_n = next(n); // cache this in case @a n is removed.
    range_min = fill->max(); // blend will be updated to one past @a fill
    ++range_min;

    // Clean up the range for @a n
    if (fill_p) {
      if (right_ext_p) {
        if (n->payload() == fill->payload()) {
          n->assign_min(fill->min());
        } else {
          n->assign_min(range_max_plus_1);
          this->insert_before(n, fill.release());
          return *this;
        }
      } else {
        // PROBLEM - not collapsing into @a pred(n) when it's adjacent / matching color.
        // But @a pred may have been removed above, not reliable at this point.
        pred = prev(n);
        if (pred && pred->payload() == fill->payload()) {
          this->remove(n);
          pred->assign_max(fill->max());
        } else {
          this->insert_before(n, fill.release());
          this->remove(n);
        }
      }
    } else {
      if (right_ext_p) {
        n->assign_min(range_max_plus_1);
        return *this;
      } else {
        this->remove(n);
      }
    }

    // Everything up to @a n.max is correct, time to process next node.
    n = next_n;
  }

  // Arriving here means there are no more ranges past @a range (those cases return from the loop).
  // Therefore the final fill node is always last in the tree.
  if (plain_color_p && range_min <= range.max()) {
    // Check if the last node can be extended to cover because it's left adjacent.
    // Can decrement @a range_min because if there's a range to the left, @a range_min is not minimal.
    n = _list.tail();
    if (n && n->max() >= --range_min && n->payload() == plain_color) {
      n->assign_max(range.max());
    } else {
      this->append(_fa.make(range_min, range.max(), plain_color));
    }
  }

  return *this;
}

} // namespace swoc
