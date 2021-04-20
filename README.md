## *prio_queue*

This is the implementation of a cache friendly priority queue as a [B-heap](https://en.wikipedia.org/wiki/B-heap).

The workings and some performance observarions are available in a post on the blog [playfulprogramming](http://playfulprogramming.blogspot.se/2015/08/cache-optimizing-priority-queue.html).

Usage differs slightly from [std::priority_queue<T>](http://en.cppreference.com/w/cpp/container/priority_queue)

The (simplified) signature is:

```Cpp
// in namespace rollbear
template <size_t miniheap_size, typename Prio, typename Value, class Compare = std::less<Prio>>
class prio_queue
{
public:
  prio_queue(Compare const& comp = Compare());
  
  using value type = Prio;
  using payload_type = Value;
  
  void                           push(Prio p, Value v);
  std::pair<Prio const&, Value&> top() const noexcept;
  void                           pop();
  void                           reschedule_top(Prio);
  bool                           empty() const noexcept;
  std::size_t                    size() const noexcept();
};
```

The reason for separate priority field and value data is that it offers much better performance than to have a priority queue over a struct/pair/tuple holding both and compare priority on it.

It is legal to set `Value` as `void`, in which case `push()` will accept only one parameter, and `top()` will return `Prio const&`, but generally performance will suffer when doing so.

`miniheap_size` is what you can tweak for optimum performance. How large to set
it depends on the size of the `Prio` data type and the size of the cache lines.
See the above mentioned
[blog post](http://playfulprogramming.blogspot.se/2015/08/cache-optimizing-priority-queue.html) for guidance.

The real signatures for `push()` uses perfect forwarding.

`reschedule_top()` is synonymous to `auto v = q.top(); q.pop(); q.push(v);`, but
is usually faster.

There is an additional allocator parameter.

If the Prio and Value types have `noexcept` move constructors and assignment, the strong exception guarantee holds, otherwise the weak exception guarantee.

Self test
---------
The included self test program relies on two external frame works.

* [Catch!](https://github.com/philsquared/Catch)
  - a header only unit test frame work
* [trompeloeil](https://github.com/rollbear/trompeloeil)
  - a header only mocking frame work

Performance benchmark
---------------------
The included performance benchmark relies on one external frame work.

* [tachymeter](https://github.com/rollbear/tachymeter)
  - a header only C++14 micro benchmark frame work
