/*
 * B-heap priority queue
 *
 * Copyright Björn Fahller 2015
 *
 *  Use, modification and distribution is subject to the
 *  Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 *
 * Project home: https://github.com/rollbear/prio_queue
 */

#ifndef ROLLBEAR_PRIO_QUEUE_HPP
#define ROLLBEAR_PRIO_QUEUE_HPP

#include <vector>
#include <cassert>
#include <tuple>


#ifdef __GNUC__
#define rollbear_prio_q_likely(x)       __builtin_expect(!!(x), 1)
#define rollbear_prio_q_unlikely(x)     __builtin_expect(!!(x), 0)
#else
  #define rollbear_prio_q_likely(x) x
  #define rollbear_prio_q_unlikely(x) x
#endif

namespace rollbear
{

namespace prio_q_internal
{
template <typename T, std::size_t block_size,
          typename Allocator = std::allocator<T>>
class skip_vector : private Allocator
{
  using A = std::allocator_traits<Allocator>;
  static constexpr std::size_t block_mask = block_size - 1;
  static_assert((block_size & block_mask) == 0U, "block size must be 2^n");
public:
           skip_vector() noexcept;
  explicit skip_vector(Allocator const &alloc)
           noexcept(std::is_nothrow_copy_constructible<T>::value);
           skip_vector(skip_vector &&v) noexcept;

  ~skip_vector() noexcept(std::is_nothrow_destructible<T>::value);

  T       &operator[](std::size_t idx) noexcept;
  T const &operator[](std::size_t idx) const noexcept;

  T           &back() noexcept;
  T const     &back() const noexcept;

  template <typename U>
  std::size_t push_back(U &&u);

  void        pop_back() noexcept(std::is_nothrow_destructible<T>::value);

  bool        empty() const noexcept;
  std::size_t size() const noexcept;
private:
  template <typename U = T>
  std::enable_if_t<std::is_pod<U>::value>
  destroy() noexcept { }

  template <typename U = T>
  std::enable_if_t<!std::is_pod<U>::value>
  destroy() noexcept(std::is_nothrow_destructible<T>::value);

  template <typename U>
  std::size_t grow(U &&u);

  template <typename U = T>
  static
  std::enable_if_t<std::is_pod<U>::value>
  move_to(T const *b, std::size_t s, T *ptr) noexcept;

  template <typename U = T>
  std::enable_if_t<
      !std::is_pod<U>::value && std::is_nothrow_move_constructible<U>::value>
  move_to(T *b, std::size_t s, T *ptr)
      noexcept(std::is_nothrow_destructible<T>::value);


  template <typename U = T>
  std::enable_if_t<!std::is_nothrow_move_constructible<U>::value>
  move_to(T const *b, std::size_t s, T *ptr)
      noexcept(std::is_nothrow_copy_constructible<T>::value
          && std::is_nothrow_destructible<T>::value);

  T           *m_ptr         = nullptr;
  std::size_t m_end          = 0;
  std::size_t m_storage_size = 0;
};


template <typename T, std::size_t block_size, typename Allocator>
skip_vector<T, block_size, Allocator>
::skip_vector() noexcept
    : skip_vector(Allocator())
{
}

template <typename T, std::size_t block_size, typename Allocator>
skip_vector<T, block_size, Allocator>
::skip_vector(Allocator const &alloc) noexcept(std::is_nothrow_copy_constructible<
    T>::value)
    : Allocator(alloc)
{
}

template <typename T, std::size_t block_size, typename Allocator>
skip_vector<T, block_size, Allocator>
::skip_vector(skip_vector &&v) noexcept
    : m_ptr(v.m_ptr)
    , m_end(v.m_end)
    , m_storage_size(v.m_storage_size)
{
  v.m_ptr = nullptr;
}


template <typename T, std::size_t block_size, typename Allocator>
skip_vector<T, block_size, Allocator>::
~skip_vector() noexcept(std::is_nothrow_destructible<T>::value)
{
  if (m_ptr)
  {
    destroy();
    A::deallocate(*this, m_ptr, m_storage_size);
  }
}

template <typename T, std::size_t block_size, typename Allocator>
T &
skip_vector<T, block_size, Allocator>::
operator[](std::size_t idx) noexcept
{
  assert(idx < m_end);
  assert((idx & block_mask) != 0);
  return m_ptr[idx];
}

template <typename T, std::size_t block_size, typename Allocator>
T const &
skip_vector<T, block_size, Allocator>::
operator[](std::size_t idx) const noexcept
{
  assert(idx < m_end);
  assert((idx & block_mask) != 0);
  return m_ptr[idx];
}

template <typename T, std::size_t block_size, typename Allocator>
T &
skip_vector<T, block_size, Allocator>::
back() noexcept
{
  assert(!empty());
  return m_ptr[m_end - 1];
}
template <typename T, std::size_t block_size, typename Allocator>
T const &
skip_vector<T, block_size, Allocator>::
back() const noexcept
{
  assert(!empty());
  return m_ptr[m_end - 1];
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::enable_if_t<!std::is_pod<U>::value>
skip_vector<T, block_size, Allocator>::
destroy() noexcept(std::is_nothrow_destructible<T>::value)
{
  auto i = m_end;
  while (rollbear_prio_q_unlikely(i-- != 0))
  {
    if (rollbear_prio_q_likely(i & block_mask))
    {
      A::destroy(*this, m_ptr + i);
    }
  }
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::size_t
skip_vector<T, block_size, Allocator>::
push_back(U &&u)
{
  if (rollbear_prio_q_likely(m_end & block_mask))
  {
    A::construct(*this, m_ptr + m_end, std::forward<U>(u));
    return m_end++;

  }
  if (rollbear_prio_q_unlikely(m_end == m_storage_size))
  {
    return grow(std::forward<U>(u));
  }
  m_end++;
  A::construct(*this, m_ptr + m_end, std::forward<U>(u));
  return m_end++;
}

template <typename T, std::size_t block_size, typename Allocator>
void
skip_vector<T, block_size, Allocator>::
pop_back() noexcept(std::is_nothrow_destructible<T>::value)
{
  assert(m_end);
  A::destroy(*this, m_ptr + --m_end);
  m_end -= (m_end & block_mask) == 1;
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::size_t
skip_vector<T, block_size, Allocator>::
grow(U &&u)
{
  auto desired_size = m_storage_size ? m_storage_size * 2 : block_size * 16;
  auto ptr          = A::allocate(*this, desired_size, m_ptr);
  auto idx          = 0;
  try
  {
    A::construct(*this, ptr + m_end + 1, std::forward<U>(u));
    idx = m_end + 1;
    if (m_storage_size)
    {
      move_to(m_ptr, m_end, ptr);
      A::deallocate(*this, m_ptr, m_storage_size);
    }
    m_ptr          = ptr;
    m_storage_size = desired_size;
    m_end          = idx + 1;
    return idx;
  }
  catch (...)
  {
    if (idx != 0) A::destroy(*this, ptr + idx);
    A::deallocate(*this, ptr, m_storage_size);
    throw;
  }
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::enable_if_t<std::is_pod<U>::value>
skip_vector<T, block_size, Allocator>::
move_to(T const *b, std::size_t s, T *ptr) noexcept
{
  std::copy(b, b + s, ptr);
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::enable_if_t<
    !std::is_pod<U>::value && std::is_nothrow_move_constructible<U>::value>
skip_vector<T, block_size, Allocator>::
move_to(T *b,
        std::size_t s,
        T *ptr) noexcept(std::is_nothrow_destructible<T>::value)
{
  for (std::size_t i = 1; rollbear_prio_q_likely(i < s); ++i)
  {
    if (rollbear_prio_q_likely(i & block_mask))
    {
      A::construct(*this, ptr + i, std::move(b[i]));
      A::destroy(*this, b + i);
    }
  }
}

template <typename T, std::size_t block_size, typename Allocator>
template <typename U>
std::enable_if_t<!std::is_nothrow_move_constructible<U>::value>
skip_vector<T, block_size, Allocator>::
move_to(T const *b, std::size_t s, T *ptr) noexcept(
std::is_nothrow_copy_constructible<T>::value
    && std::is_nothrow_destructible<T>::value)
{
  std::size_t i;
  try
  {
    for (i = 1; rollbear_prio_q_likely(i != s); ++i)
    {
      if (rollbear_prio_q_likely(i & block_mask))
      {
        A::construct(*this, ptr + i, b[i]);
      }
    }
    while (rollbear_prio_q_likely(i--))
    {
      if (rollbear_prio_q_likely(i & block_mask))
      {
        A::destroy(*this, b + i);
      }
    }
  }
  catch (...)
  {
    while (rollbear_prio_q_likely(i--))
    {
      if (rollbear_prio_q_likely(i & block_mask))
      {
        A::destroy(*this, ptr + i);
      }
    }
    throw;
  }
}

template <typename T, std::size_t block_size, typename Allocator>
bool
skip_vector<T, block_size, Allocator>::empty() const noexcept
{
  return size() == 0;
}

template <typename T, std::size_t block_size, typename Allocator>
std::size_t
skip_vector<T, block_size, Allocator>::size() const noexcept
{
  return m_end;
}

template <std::size_t blocking>
struct heap_heap_addressing
{
  static const constexpr std::size_t block_size = blocking;
  static const constexpr std::size_t block_mask = block_size - 1;
  static_assert((block_size & block_mask) == 0U,
                "block size must be 2^n for some integer n");

  static std::size_t child_of(std::size_t node_no) noexcept;
  static std::size_t parent_of(std::size_t node_no) noexcept;
  static bool        is_block_root(std::size_t node_no) noexcept;
  static std::size_t block_offset(std::size_t node_no) noexcept;
  static std::size_t block_base(std::size_t node_no) noexcept;
  static bool        is_block_leaf(std::size_t node_no) noexcept;
  static std::size_t child_no(std::size_t node_no) noexcept;
};

template <std::size_t block_size, typename V,
                                  typename Allocator = std::allocator<V>>
class payload
{
public:
  payload(Allocator const &alloc = Allocator{ }) : m_storage(alloc) { }
  template <typename U>
  void push_back(U &&u) { m_storage.push_back(std::forward<U>(u)); }
  void pop_back() { m_storage.pop_back(); }
  V &top() { return m_storage[1]; }
  V &back() { return m_storage.back(); }
  void store(std::size_t idx, V &&v) { m_storage[idx] = std::move(v); }
  void move(std::size_t from, std::size_t to)
  {
    m_storage[to] = std::move(m_storage[from]);
  }
private:
  skip_vector<V, block_size, Allocator> m_storage;
};

template <std::size_t block_size, typename Allocator>
class payload<block_size, void, Allocator>
{
public:
  payload(Allocator const & = Allocator{ }) { }
  constexpr bool back() const { return true; }
  constexpr void store(std::size_t, bool) const { }
  constexpr void move(std::size_t, std::size_t) const { }
  constexpr void pop_back() const { };
};

} // namespace prio_q_internal

template <std::size_t block_size, typename T, typename V,
                                  typename Compare = std::less<T>,
                                  typename Allocator = std::allocator<T>>
class prio_queue : private Compare, private prio_q_internal::payload<block_size, V>
{
  using address = prio_q_internal::heap_heap_addressing<block_size>;
  using P = prio_q_internal::payload<block_size, V>;
public:
  prio_queue(Compare const &compare = Compare()) : Compare(compare) { }
  explicit prio_queue(Compare const &compare, Allocator const &a)
      : Compare(compare)
      , m_storage(a) { }

  using value_type = T;
  using payload_type = V;

  template <typename U, typename X = V>
  std::enable_if_t<std::is_same<X, void>::value>
  push(U &&u);

  template <typename U, typename X>
  std::enable_if_t<!std::is_same<X, void>::value>
  push(U &&key, X &&value);

  template <typename U = V>
  std::enable_if_t<std::is_same<U, void>::value, value_type const &>
  top() const noexcept;

  template <typename U = V>
  std::enable_if_t<!std::is_same<U, void>::value, std::pair<T const &, U &>>
  top() noexcept;

  void pop() noexcept(std::is_nothrow_destructible<T>::value);

  bool empty() const noexcept;

  std::size_t size() const noexcept;
private:
  template <typename U>
  void push_key(U &&key);

  bool sorts_before(value_type const &lv, value_type const &rv) const noexcept;

  prio_q_internal::skip_vector<T, block_size, Allocator> m_storage;
};


template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
template <typename U, typename X>
inline
std::enable_if_t<std::is_same<X, void>::value>
prio_queue<block_size, T, V, Compare, Allocator>::
push(U &&u)
{
  push_key(std::forward<U>(u));
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
template <typename U, typename X>
inline
std::enable_if_t<!std::is_same<X, void>::value>
prio_queue<block_size, T, V, Compare, Allocator>::
push(U &&key, X &&value)
{
  P::push_back(std::forward<X>(value));
  push_key(std::forward<U>(key));
}


template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
template <typename U>
inline
void
prio_queue<block_size, T, V, Compare, Allocator>::
push_key(U &&key)
{
  auto hole_idx = m_storage.push_back(std::forward<U>(key));
  auto tmp      = std::move(m_storage.back());
  auto val      = std::move(P::back());

  while (rollbear_prio_q_likely(hole_idx != 1U))
  {
    auto parent = address::parent_of(hole_idx);
    auto &p     = m_storage[parent];
    if (rollbear_prio_q_likely(!sorts_before(tmp, p))) break;
    m_storage[hole_idx] = std::move(p);
    P::move(parent, hole_idx);
    hole_idx = parent;
  }
  m_storage[hole_idx] = std::move(tmp);
  P::store(hole_idx, std::move(val));
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
inline
void
prio_queue<block_size, T, V, Compare, Allocator>::
pop()
noexcept(std::is_nothrow_destructible<T>::value)
{
  assert(!empty());
  std::size_t idx      = 1;
  auto const  last_idx = m_storage.size() - 1;
  for (; ;)
  {
    auto lc = address::child_of(idx);
    if (rollbear_prio_q_unlikely(lc > last_idx)) break;
    auto const sibling_offset = rollbear_prio_q_unlikely(address::is_block_leaf(idx))
                                ? address::block_size : 1;
    auto       rc             = lc + sibling_offset;
    auto       i              =
                   rc < last_idx && !sorts_before(m_storage[lc], m_storage[rc]);
    auto       next           = i ? rc : lc;
    m_storage[idx] = std::move(m_storage[next]);
    P::move(next, idx);
    idx = next;
  }
  if (rollbear_prio_q_likely(idx != last_idx))
  {
    auto last     = std::move(m_storage.back());
    auto last_val = std::move(P::back());
    while (rollbear_prio_q_likely(idx != 1))
    {
      auto parent = address::parent_of(idx);
      if (rollbear_prio_q_likely(!sorts_before(last, m_storage[parent]))) break;
      m_storage[idx] = std::move(m_storage[parent]);
      P::move(parent, idx);
      idx = parent;
    }
    m_storage[idx] = std::move(last);
    P::store(idx, std::move(last_val));
  }
  m_storage.pop_back();
  P::pop_back();
}


template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
template <typename U>
inline
std::enable_if_t<std::is_same<U, void>::value, T const &>
prio_queue<block_size, T, V, Compare, Allocator>::
top()
const
noexcept
{
  assert(!empty());
  return m_storage[1];
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
template <typename U>
inline
std::enable_if_t<!std::is_same<U, void>::value, std::pair<T const &, U &>>
prio_queue<block_size, T, V, Compare, Allocator>::
top()
noexcept
{
  assert(!empty());
  return { m_storage[1], P::top() };
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
inline
bool
prio_queue<block_size, T, V, Compare, Allocator>::
empty()
const
noexcept
{
  return m_storage.empty();
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
inline
std::size_t
prio_queue<block_size, T, V, Compare, Allocator>::
size()
const
noexcept
{
  return m_storage.size()
      - (m_storage.size() + address::block_size - 1) / address::block_size;
}

template <std::size_t block_size, typename T, typename V, typename Compare,
                                  typename Allocator>
inline
bool
prio_queue<block_size, T, V, Compare, Allocator>::
sorts_before(value_type const &lv, value_type const &rv)
const
noexcept
{
  Compare const &c = *this;
  return c(lv, rv);
}

namespace prio_q_internal
{

template <std::size_t blocking>
inline
std::size_t
heap_heap_addressing<blocking>::
child_of(std::size_t node_no)
noexcept
{
  if (rollbear_prio_q_likely(!is_block_leaf(node_no)))
  {
    return node_no + block_offset(node_no);
  }
  auto base = block_base(node_no) + 1;
  return base * block_size + child_no(node_no) * block_size * 2 + 1;
}

template <std::size_t blocking>
inline
std::size_t
heap_heap_addressing<blocking>::
parent_of(std::size_t node_no)
noexcept
{
  auto const node_root = block_base(node_no); // 16
  if (rollbear_prio_q_likely(!is_block_root(node_no)))
  {
    return node_root + block_offset(node_no) / 2;
  }
  auto const parent_base = block_base(node_root / block_size - 1); // 0
  auto const child       =
                 ((node_no - block_size) / block_size - parent_base) / 2;
  return parent_base + block_size / 2 + child; // 30
}

template <std::size_t blocking>
inline
bool
heap_heap_addressing<blocking>::
is_block_root(std::size_t node_no)
noexcept
{
  return block_offset(node_no) == 1U;
}

template <std::size_t blocking>
inline
std::size_t
heap_heap_addressing<blocking>::
block_offset(std::size_t node_no)
noexcept
{
  return node_no & block_mask;
}

template <std::size_t blocking>
inline
std::size_t
heap_heap_addressing<blocking>::
block_base(std::size_t node_no)
noexcept
{
  return node_no & ~block_mask;
}

template <std::size_t blocking>
inline
bool
heap_heap_addressing<blocking>::
is_block_leaf(std::size_t node_no)
noexcept
{
  return (node_no & (block_size >> 1)) != 0U;
}

template <std::size_t blocking>
inline
std::size_t
heap_heap_addressing<blocking>::
child_no(std::size_t node_no)
noexcept
{
  assert(is_block_leaf(node_no));
  return node_no & (block_mask >> 1);
}
} // namespace prio_q_internal


} // namespace rollbear

#undef rollbear_prio_q_likely
#undef rollbear_prio_q_unlikely

#endif //ROLLBEAR_PRIO_QUEUE_HPP
