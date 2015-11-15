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


#include "prio_queue.hpp"
#include <queue>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

using A = rollbear::prio_q_internal::heap_heap_addressing<8>;
using V = rollbear::prio_q_internal::skip_vector<int, 4>;
using rollbear::prio_queue;

TEST_CASE("a default constructed vector is empty", "[vector]")
{
  V v;
  REQUIRE(v.size() == 0);
  REQUIRE(v.empty());
}

TEST_CASE("a has size 2 after one push_key", "[vector]")
{
  V v;
  auto i = v.push_back(1);
  REQUIRE(!v.empty());
  REQUIRE(v.size() == 2);
  REQUIRE(i == 1);
}

TEST_CASE("a vector of size 2 becomes empty on pop", "[vector]")
{
  V v;
  v.push_back(1);
  v.pop_back();
  REQUIRE(v.empty());
  REQUIRE(v.size() == 0);
}

TEST_CASE("push_key indexes skip multiples of 4", "[vector]")
{
  V v;
  REQUIRE(v.push_back(1) == 1);
  REQUIRE(v.push_back(1) == 2);
  REQUIRE(v.push_back(1) == 3);
  REQUIRE(v.push_back(1) == 5);
  REQUIRE(v.push_back(1) == 6);
  REQUIRE(v.push_back(1) == 7);
  REQUIRE(v.push_back(1) == 9);
}

TEST_CASE("back refers to last element through push_key and pop", "[vector]")
{
  V v;
  v.push_back(21);
  REQUIRE(v.back() == 21);
  v.push_back(20);
  REQUIRE(v.back() == 20);
  v.push_back(19);
  REQUIRE(v.back() == 19);
  v.push_back(18);
  REQUIRE(v.back() == 18);
  v.push_back(17);
  REQUIRE(v.back() == 17);
  v.pop_back();
  REQUIRE(v.back() == 18);
  v.pop_back();
  REQUIRE(v.back() == 19);
  v.pop_back();
  REQUIRE(v.back() == 20);
  v.pop_back();
  REQUIRE(v.back() == 21);
  v.pop_back();
  REQUIRE(v.empty());

}
TEST_CASE("block root", "[addressing]")
{
  REQUIRE(A::is_block_root(1));
  REQUIRE(A::is_block_root(9));
  REQUIRE(A::is_block_root(17));
  REQUIRE(A::is_block_root(73));
  REQUIRE(!A::is_block_root(2));
  REQUIRE(!A::is_block_root(3));
  REQUIRE(!A::is_block_root(4));
  REQUIRE(!A::is_block_root(7));
  REQUIRE(!A::is_block_root(31));
}

TEST_CASE("block leaf", "[addressing]")
{
  REQUIRE(!A::is_block_leaf(1));
  REQUIRE(!A::is_block_leaf(2));
  REQUIRE(!A::is_block_leaf(3));
  REQUIRE(A::is_block_leaf(4));
  REQUIRE(A::is_block_leaf(5));
  REQUIRE(A::is_block_leaf(6));
  REQUIRE(A::is_block_leaf(7));
  REQUIRE(A::is_block_leaf(28));
  REQUIRE(A::is_block_leaf(29));
  REQUIRE(A::is_block_leaf(30));
  REQUIRE(!A::is_block_leaf(257));
  REQUIRE(A::is_block_leaf(255));
}
TEST_CASE("Obtaining child", "[addressing]")
{
  REQUIRE(A::child_of(1) == 2);
  REQUIRE(A::child_of(2) == 4);
  REQUIRE(A::child_of(3) == 6);
  REQUIRE(A::child_of(4) == 9);
  REQUIRE(A::child_of(31) == 249);
}

TEST_CASE("Obtaining parent", "[addressing]")
{
  REQUIRE(A::parent_of(2) == 1);
  REQUIRE(A::parent_of(3) == 1);
  REQUIRE(A::parent_of(6) == 3);
  REQUIRE(A::parent_of(7) == 3);
  REQUIRE(A::parent_of(9) == 4);
  REQUIRE(A::parent_of(17) == 4);
  REQUIRE(A::parent_of(33) == 5);
  REQUIRE(A::parent_of(29) == 26);
  REQUIRE(A::parent_of(1097) == 140);
}

TEST_CASE("a default constructed queue is empty", "[empty]")
{
  prio_queue<16, int, void> q;
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);
}

TEST_CASE("an empty queue is not empty when one element is inserted", "[empty]")
{
  prio_queue<16, int, void> q;
  q.push(1);
  REQUIRE(!q.empty());
  REQUIRE(q.size() == 1);
}

TEST_CASE("a queue with one element has it on top", "[single element]")
{
  prio_queue<16, int, void> q;
  q.push(8);
  REQUIRE(q.top() == 8);
}

TEST_CASE("a queue with one element becomes empty when popped",
          "[single element],[empty]")
{
  prio_queue<16, int, void> q;
  q.push(9);
  q.pop();
  REQUIRE(q.empty());
  REQUIRE(q.size() == 0);
}

TEST_CASE("insert sorted stays sorted", "[dead]")
{
  prio_queue<16, int, void> q;
  q.push(1);
  q.push(2);
  q.push(3);
  q.push(4);
  q.push(5);
  q.push(6);
  q.push(7);
  q.push(8);
  REQUIRE(q.top() == 1);
  q.pop();
  REQUIRE(q.top() == 2);
  q.pop();
  REQUIRE(q.top() == 3);
  q.pop();
  REQUIRE(q.top() == 4);
  q.pop();
  REQUIRE(q.top() == 5);
  q.pop();
  REQUIRE(q.top() == 6);
  q.pop();
  REQUIRE(q.top() == 7);
  q.pop();
  REQUIRE(q.top() == 8);
  q.pop();
  REQUIRE(q.empty());
}

TEST_CASE("key value pairs go in tandem", "[nontrivial]")
{
  struct P
  {
    int a;
    int b;
    bool operator==(const std::pair<int const&, int&>& rh) const
    {
      return a == rh.first && b == rh.second;
    }
  };
  prio_queue<16, int, int> q;
  q.push(3, -3);
  q.push(4, -4);
  q.push(8, -8);
  q.push(1, -1);
  q.push(22, -22);
  q.push(23, -23);
  q.push(16, -16);
  q.push(9, -9);
  q.push(25, -25);
  q.push(20, -20);
  q.push(10, -10);
  q.push(5, -5);
  q.push(11, -11);
  q.push(12, -12);
  q.push(19, -19);
  q.push(2, -2);

  REQUIRE((P{1, -1}) == q.top());
  q.pop();
  REQUIRE((P{2, -2}) == q.top());
  q.pop();
  REQUIRE((P{3, -3}) == q.top());
  q.pop();
  REQUIRE((P{4, -4}) == q.top());
  q.pop();
  REQUIRE((P{5, -5}) == q.top());
  q.pop();
  REQUIRE((P{8, -8}) == q.top());
  q.pop();
  REQUIRE((P{9, -9}) == q.top());
  q.pop();
  REQUIRE((P{10, -10}) == q.top());
  q.pop();
  REQUIRE((P{11, -11}) == q.top());
  q.pop();
  REQUIRE((P{12, -12}) == q.top());
  q.pop();
  REQUIRE((P{16, -16}) == q.top());
  q.pop();
  REQUIRE((P{19, -19}) == q.top());
  q.pop();
  REQUIRE((P{20, -20}) == q.top());
  q.pop();
  REQUIRE((P{22, -22}) == q.top());
  q.pop();
  REQUIRE((P{23, -23}) == q.top());
  q.pop();
  REQUIRE((P{25, -25}) == q.top());
  q.pop();
  REQUIRE(q.empty());

}

TEST_CASE("key value pairs can have complex value type", "[nontrivial]")
{
  prio_queue<16, int, std::unique_ptr<int>> q;
  q.push(2, nullptr);
  q.push(1, nullptr);
  REQUIRE(q.top().first == 1);
  q.pop();
  REQUIRE(q.top().first == 2);
  q.pop();
  REQUIRE(q.empty());
}
TEST_CASE("randomly inserted elements are popped sorted", "heap")
{
  prio_queue<16, int, void> q;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(1,100000);
  int n[36000];
  for (auto& i : n)
  {
    i = dist(gen);
    q.push(i);
  }

  REQUIRE(!q.empty());
  REQUIRE(q.size() == 36000);
  std::sort(std::begin(n), std::end(n));
  for (auto i : n)
  {
    REQUIRE(q.top() == i);
    q.pop();
  }
  REQUIRE(q.empty());
}

TEST_CASE("reschedule top with highest prio leaves order unchanged", "heap")
{
  prio_queue<4, int, int*> q;
  //              0  1   2   3  4   5  6   7   8
  int nums[] = { 32, 1, 88, 16, 9, 11, 3, 22, 23 };
  for (auto& i : nums) q.push(i, &i);
  REQUIRE(q.top().first == 1);
  REQUIRE(q.top().second == &nums[1]);
  REQUIRE(*q.top().second == 1);

  q.reschedule_top(2);

  REQUIRE(q.top().first == 2);
  REQUIRE(q.top().second == &nums[1]);
  q.pop();
  REQUIRE(q.top().first == 3);
  REQUIRE(q.top().second == &nums[6]);
  q.pop();
  REQUIRE(q.top().first == 9);
  REQUIRE(q.top().second == &nums[4]);
  q.pop();
  REQUIRE(q.top().first == 11);
  REQUIRE(q.top().second == &nums[5]);
  q.pop();
  REQUIRE(q.top().first == 16);
  REQUIRE(q.top().second == &nums[3]);
  q.pop();
  REQUIRE(q.top().first == 22);
  REQUIRE(q.top().second == &nums[7]);
  q.pop();
  REQUIRE(q.top().first == 23);
  REQUIRE(q.top().second == &nums[8]);
  q.pop();
  REQUIRE(q.top().first == 32);
  REQUIRE(q.top().second == &nums[0]);
  q.pop();
  REQUIRE(q.top().first == 88);
  REQUIRE(q.top().second == &nums[2]);
  q.pop();
  REQUIRE(q.empty());
}

TEST_CASE("reschedule to mid range moves element to correct place", "heap")
{
  prio_queue<4, int, int*> q;
  //              0  1   2   3  4   5  6   7   8
  int nums[] = { 32, 1, 88, 16, 9, 11, 3, 22, 23 };
  for (auto& i : nums) q.push(i, &i);
  REQUIRE(q.top().first == 1);
  REQUIRE(q.top().second == &nums[1]);
  REQUIRE(*q.top().second == 1);

  q.reschedule_top(12);

  REQUIRE(q.top().first == 3);
  REQUIRE(q.top().second == &nums[6]);
  q.pop();
  REQUIRE(q.top().first == 9);
  REQUIRE(q.top().second == &nums[4]);
  q.pop();
  REQUIRE(q.top().first == 11);
  REQUIRE(q.top().second == &nums[5]);
  q.pop();
  REQUIRE(q.top().first == 12);
  REQUIRE(q.top().second == &nums[1]);
  q.pop();
  REQUIRE(q.top().first == 16);
  REQUIRE(q.top().second == &nums[3]);
  q.pop();
  REQUIRE(q.top().first == 22);
  REQUIRE(q.top().second == &nums[7]);
  q.pop();
  REQUIRE(q.top().first == 23);
  REQUIRE(q.top().second == &nums[8]);
  q.pop();
  REQUIRE(q.top().first == 32);
  REQUIRE(q.top().second == &nums[0]);
  q.pop();
  REQUIRE(q.top().first == 88);
  REQUIRE(q.top().second == &nums[2]);
  q.pop();
  REQUIRE(q.empty());
}

TEST_CASE("reschedule to last moves element to correct place", "heap")
{
  prio_queue<4, int, int*> q;
  //              0  1   2   3  4   5  6   7   8
  int nums[] = { 32, 1, 88, 16, 9, 11, 3, 22, 23 };
  for (auto& i : nums) q.push(i, &i);
  REQUIRE(q.top().first == 1);
  REQUIRE(q.top().second == &nums[1]);
  REQUIRE(*q.top().second == 1);

  q.reschedule_top(89);

  REQUIRE(q.top().first == 3);
  REQUIRE(q.top().second == &nums[6]);
  q.pop();
  REQUIRE(q.top().first == 9);
  REQUIRE(q.top().second == &nums[4]);
  q.pop();
  REQUIRE(q.top().first == 11);
  REQUIRE(q.top().second == &nums[5]);
  q.pop();
  REQUIRE(q.top().first == 16);
  REQUIRE(q.top().second == &nums[3]);
  q.pop();
  REQUIRE(q.top().first == 22);
  REQUIRE(q.top().second == &nums[7]);
  q.pop();
  REQUIRE(q.top().first == 23);
  REQUIRE(q.top().second == &nums[8]);
  q.pop();
  REQUIRE(q.top().first == 32);
  REQUIRE(q.top().second == &nums[0]);
  q.pop();
  REQUIRE(q.top().first == 88);
  REQUIRE(q.top().second == &nums[2]);
  q.pop();
  REQUIRE(q.top().first == 89);
  REQUIRE(q.top().second == &nums[1]);
  q.pop();
  REQUIRE(q.empty());
}

TEST_CASE("reschedule top of 2 elements to last", "[heap]")
{
  prio_queue<8, int, void> q;
  q.push(1);
  q.push(2);
  REQUIRE(q.top() == 1);
  q.reschedule_top(3);
  REQUIRE(q.top() == 2);
}

TEST_CASE("reschedule top of 3 elements left to 2nd", "[heap]")
{
  prio_queue<8, int, void> q;
  q.push(1);
  q.push(2);
  q.push(4);
  REQUIRE(q.top() == 1);
  q.reschedule_top(3);
  REQUIRE(q.top() == 2);
}

TEST_CASE("reschedule top of 3 elements right to 2nd", "[heap]")
{
  prio_queue<8, int, void> q;
  q.push(1);
  q.push(4);
  q.push(2);
  REQUIRE(q.top() == 1);
  q.reschedule_top(3);
  REQUIRE(q.top() == 2);
}


TEST_CASE("reschedule top random gives same resultas pop/push", "[heap]")
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned> dist(1,100000);

  prio_queue<8, unsigned, void> pq;
  std::priority_queue<unsigned, std::vector<unsigned>, std::greater<>> stdq;

  for (size_t outer = 0; outer < 100U; ++outer)
  {
    unsigned num = gen();
    pq.push(num);
    stdq.push(num);
    for (size_t inner = 0; inner < 100; ++inner)
    {
      unsigned newval = gen();
      pq.reschedule_top(newval);
      stdq.pop();
      stdq.push(newval);
      auto n = pq.top();
      auto sn = stdq.top();
      REQUIRE(sn == n);
    }
  }
}

struct ptr_cmp
{
  template <typename T>
  bool operator()(T const& lh, T const& rh) const { return *lh < *rh;}
};

TEST_CASE("unique ptrs are sorted with custom compare", "[nontrivial]")
{
  prio_queue<8, std::unique_ptr<int>, void, ptr_cmp> q;
  for (int i = 255; i >= 0; --i)
  {
    q.push(std::make_unique<int>(i));
  }

  for (int i = 0; i < 256; ++i)
  {
    REQUIRE(*q.top() == i);
    q.pop();
  }
  REQUIRE(q.empty());
}

unsigned obj_count;
unsigned copy_count;
unsigned move_throw_count;
unsigned copy_throw_count;
struct traced_throwing_move
{
  traced_throwing_move(int n_) : n{n_} { ++obj_count; }
  traced_throwing_move(const traced_throwing_move& rh) :n{rh.n} { if (--copy_throw_count == 0) throw 2; ++obj_count; ++copy_count; }
  traced_throwing_move(traced_throwing_move&& rh) : n{rh.n} { if (--move_throw_count == 0) throw 3; ++obj_count; rh.n = -rh.n;}
  ~traced_throwing_move() { --obj_count;}
  traced_throwing_move& operator=(const traced_throwing_move& rh) { n = rh.n;return *this; }
  int n;
  bool operator<(const traced_throwing_move& rh) const { return n < rh.n;}
};

TEST_CASE("throwable move ctor causes copies", "[nontrivial]")
{
  obj_count = 0;
  move_throw_count = 0;
  copy_throw_count = 0;
  copy_count = 0;
  {
    prio_queue<16, traced_throwing_move, void> q;
    for (int i = 0; i < 15*16; ++i)
    {
      q.push(500 - i);
    }
    REQUIRE(obj_count == 15*16);
    REQUIRE(copy_count == 0);
    q.push(100);
    REQUIRE(obj_count == 15*16 + 1);
    REQUIRE(copy_count == 15*16);
  }

  REQUIRE(obj_count == 0);
  REQUIRE(copy_count == 15*16);
}

TEST_CASE("throwing move allows safe destruction", "[nontrivial")
{
  obj_count = 0;
  move_throw_count = 18;
  copy_throw_count = 0;
  copy_count = 0;
  bool too_soon = true;
  try {
    prio_queue<16, traced_throwing_move, void> q;
    for (int i = 0; i < 17; ++i)
    {
      q.push(i);
    }
    too_soon = false;
    q.push(18);
    FAIL("didn't throw");
  }
  catch (int)
  {
  }
  REQUIRE_FALSE(too_soon);
  REQUIRE(obj_count == 0);
}

TEST_CASE("throwing copy allows safe destruction", "[nontrivial]")
{
  obj_count = 0;
  move_throw_count = 0;
  copy_throw_count = 15*15;
  copy_count = 0;
  std::cout << "begin\n";
  bool too_soon = true;
  try {
    prio_queue<16, traced_throwing_move, void> q;
    for (int i = 0; i < 15*16; ++i)
    {
      q.push(i);
    }
    too_soon = false;
    q.push(1000);
    FAIL("didn't throw");
  }
  catch (int)
  {
  }
  REQUIRE_FALSE(too_soon);
  REQUIRE(obj_count == 0);
}
