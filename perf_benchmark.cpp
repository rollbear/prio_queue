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
#include <tachymeter/benchmark.hpp>
#include <tachymeter/seq.hpp>
#include <tachymeter/CSV_reporter.hpp>
#include <queue>
#include <random>
#include <chrono>
#include <algorithm>
#include <iostream>

#include <memory>
#include <sstream>
using namespace std::literals::chrono_literals;
using namespace tachymeter;
using Clock = std::chrono::high_resolution_clock;
using rollbear::prio_queue;

struct null_obj_t
{
  constexpr operator int() const { return 0; }
  template <typename T>
  constexpr operator std::unique_ptr<T>() const { return nullptr; }
};

static const constexpr null_obj_t null_obj{ };
static int n[600000];
auto const test_sizes        = powers(seq(1, 2, 5), 1, 100000, 10);
auto const min_test_duration = 1000ms;

template <typename T>
struct is_pair
{
  struct no {};
  static no func(...);
  struct yes {};
  template <typename U, typename V>
  static yes func(std::pair<U, V> const*);
  static constexpr bool value = std::is_same<yes, decltype(func(std::declval<T*>()))>::value;
};

template <typename T, typename ... V>
struct has_push
{
  struct no {};
  static no func(...);
  template <typename U>
  static auto func(U* u) -> decltype(u->push(std::declval<V>()...));
  static constexpr bool value = !std::is_same<no, decltype(func(std::declval<T*>()))>::value;
};

template <typename Q>
inline
std::enable_if_t<is_pair<typename Q::value_type>::value>
add(Q& q, int n)
{
  q.push(typename Q::value_type(n, null_obj));
}

template <typename Q>
inline
std::enable_if_t<!is_pair<typename Q::value_type>::value && has_push<Q, int>::value>
add(Q& q, int n)
{
  q.push(n);
}


template <typename Q>
inline
std::enable_if_t<!is_pair<typename Q::value_type>::value && !has_push<Q, int>::value>
add(Q& q, int n)
{
  q.push(n, null_obj);
}

template <typename Q>
class populate
{
public:
  populate(uint64_t) { }
  void operator()(uint64_t size)
  {
    for (uint64_t i = 0; i != size; ++i)
    {
      add(q, n[i]);
    }
  }
private:
  Q q;
};

template <typename Q>
class pop_all
{
public:
  pop_all(std::size_t size)
  {
    for (uint64_t i = 0; i != size; ++i)
    {
      add(q, n[i]);
    }
  }
  void operator()(uint64_t size)
  {
    while (size--)
    {
      q.pop();
    }
  }
private:
  Q q;
};

template <typename Q, uint64_t delta_size, uint64_t num_cycles>
class operate
{
public:
  operate(std::size_t size)
  {
    for (uint64_t i = 0; i != size; ++i)
    {
      add(q, n[i]);
    }
  }
  void operator()(uint64_t size)
  {
    auto p                = n + size;
    auto remaining_cycles = num_cycles;
    while (remaining_cycles--)
    {
      auto elements_to_push = delta_size;
      while (elements_to_push--)
      {
        add(q, *p++);
      }
      auto elements_to_pop = delta_size;
      while (elements_to_pop--)
      {
        q.pop();
      }
    }
  }
private:
  Q q;
};


inline
bool operator<(const std::pair<int, std::unique_ptr<int>> &lh,
               const std::pair<int, std::unique_ptr<int>> &rh)
{
  return lh.first < rh.first;
}

template <std::size_t size>
void measure_prio_queue(int argc, char *argv[])
{
  std::ostringstream os;
  os << "/tmp/q/" << size;
  std::string path = os.str();

  std::cout << path << '\n';

  CSV_reporter     reporter(path.c_str(), &std::cout);
  benchmark<Clock> benchmark(reporter);

  using qint = prio_queue<8, int, void>;
  using qintintp = prio_queue<size, std::pair<int, int>, void>;
  using qintptrp = prio_queue<size, std::pair<int, std::unique_ptr<int>>, void>;
  using qintp = prio_queue<size, int, std::unique_ptr<int>>;
  using qintint = prio_queue<size, int, int>;

  using std::to_string;

  benchmark.measure<populate<qint>>(test_sizes,
                                    "populate prio_queue<int,void>",
                                    min_test_duration);
  benchmark.measure<pop_all<qint>>(test_sizes,
                                   "pop all prio_queue<int,void>",
                                   min_test_duration);
  benchmark.measure<operate<qint, 320, 200>>(test_sizes,
                                           "operate prio_queue<int,void>",
                                           min_test_duration);

  benchmark.measure<populate<qintintp>>(test_sizes,
                                        "populate prio_queue<<int,int>, void>",
                                        min_test_duration);
  benchmark.measure<pop_all<qintintp>>(test_sizes,
                                       "pop all prio_queue<<int,int>, void>",
                                       min_test_duration);
  benchmark.measure<operate<qintintp, 320, 200>>(test_sizes,
                                               "operate prio_queue<<int,int>, void>",
                                               min_test_duration);

  benchmark.measure<populate<qintptrp>>(test_sizes,
                                        "populate prio_queue<<int,ptr>, void>",
                                        min_test_duration);
  benchmark.measure<pop_all<qintptrp>>(test_sizes,
                                       "pop all prio_queue<<int,ptr>, void>",
                                       min_test_duration);
  benchmark.measure<operate<qintptrp, 320, 200>>(test_sizes,
                                               "operate prio_queue<<int,ptr>, void>",
                                               min_test_duration);


  benchmark.measure<populate<qintint>>(test_sizes,
                                       "populate prio_queue<int,int>",
                                       min_test_duration);
  benchmark.measure<pop_all<qintint>>(test_sizes,
                                      "pop all prio_queue<int,int>",
                                      min_test_duration);
  benchmark.measure<operate<qintint, 320, 200>>(test_sizes,
                                              "operate prio_queue<int,int>",
                                              min_test_duration);


  benchmark.measure<populate<qintp>>(test_sizes,
                                     "populate prio_queue<int,ptr>",
                                     min_test_duration);
  benchmark.measure<pop_all<qintp>>(test_sizes,
                                    "pop all prio_queue<int,ptr>",
                                    min_test_duration);
  benchmark.measure<operate<qintp, 320, 200>>(test_sizes,
                                            "operate prio_queue<int,ptr>",
                                            min_test_duration);
  benchmark.run(argc, argv);
}

int main(int argc, char *argv[])
{
  std::random_device              rd;
  std::mt19937                    gen(rd());
  std::uniform_int_distribution<> dist(1, 10000000);
  std::fill(std::begin(n), std::end(n), dist(gen));


  std::cout << sizeof(int) << ' '
      << sizeof(std::pair<int, std::unique_ptr<int>>) << '\n';

  measure_prio_queue<8>(argc, argv);
  measure_prio_queue<16>(argc, argv);
  measure_prio_queue<32>(argc, argv);
  measure_prio_queue<64>(argc, argv);


  using qint = std::priority_queue<int>;
  using qintintp = std::priority_queue<std::pair<int, int>>;
  using qintptrp = std::priority_queue<std::pair<int, std::unique_ptr<int>>>;


  CSV_reporter     reporter("/tmp/q/std", &std::cout);
  benchmark<Clock> benchmark(reporter);

  benchmark.measure<populate<qint>>(test_sizes,
                                    "populate priority_queue<int>",
                                    min_test_duration);
  benchmark.measure<pop_all<qint>>(test_sizes,
                                   "pop all priority_queue<int>",
                                   min_test_duration);
  benchmark.measure<operate<qint, 320, 200>>(test_sizes,
                                           "operate priority_queue<int>",
                                           min_test_duration);


  benchmark.measure<populate<qintintp>>(test_sizes,
                                    "populate priority_queue<<int,int>>",
                                    min_test_duration);
  benchmark.measure<pop_all<qintintp>>(test_sizes,
                                   "pop all priority_queue<<int,int>>",
                                   min_test_duration);
  benchmark.measure<operate<qintintp, 320, 200>>(test_sizes,
                                           "operate priority_queue<<int,int>>",
                                           min_test_duration);

  benchmark.measure<populate<qintptrp>>(test_sizes,
                                        "populate priority_queue<<int,ptr>>",
                                        min_test_duration);
  benchmark.measure<pop_all<qintptrp>>(test_sizes,
                                       "pop all priority_queue<<int,ptr>>",
                                       min_test_duration);
  benchmark.measure<operate<qintptrp, 320, 200>>(test_sizes,
                                               "operate priority_queue<<int,ptr>>",
                                               min_test_duration);

  benchmark.run(argc, argv);
}
