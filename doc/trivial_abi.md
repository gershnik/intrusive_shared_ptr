When built with CLang compiler `intrusive_shared_ptr` is marked with [\[\[clang::trivial_abi\]\]](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi) attribute. A good description of what this attribute does and why it is important
for performance can be found [here](https://quuxplusone.github.io/blog/2018/05/02/trivial-abi-101/).
Another take on the performance issue as a comment on standard library proposal can be found 
[here](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1351r0.html#params).

However, using trivial ABI with a type that has non trivial destructor (and of course a smart pointer destructor is non-trivial - it has to decrement reference count!) imeddiately raises serious objections. Such use makes the
destructor run *out of order* inside the called function whereas destructors of other parameters run after the
function exits. While on theoretical level this is indeed wrong, the interesting question is whether it can ever matter
in practice. 
An important observation is that the fact that destructor runs out of order only really matters in one circumstance - 
when the smart pointer object holds the last reference to pointee and its destructor causes the pointee to be
destroyed. This out of order destruction can be, in principle, observed by outside code. Or can it?
If the smart pointer passed as a function argument holds the last reference then, for some other code to observe the
pointee demise, it would need to refer to it via an unsafe raw pointer and only while the smart pointer is alive.
Something like this

```cpp

T * raw = ...;
struct nasty
{
    nasty(T * p): _p(p) {}
    ~nasty()
    {
        //use p-> here
    }
};
foo(smart_ptr<T>(intrusive_noref(raw)), nasty(raw));

```

Assuming **left to right order of evaluation** for function arguments, this indeed will do bad things in `nasty` destructor.
This is indeed what happens with clang on x64 MacOS.
But wait a minute, even though there is a [proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0145r3.pdf) 
to fix the order of function arguments evaluation this is not yet part of the standard and you cannot rely on a specific
order here. The code is actually broken even without trivial ABI. And indeed it is on GCC 7.4 on x64 Ubuntu.
Even if the order of evaluation becomes fixed in some future C++ standard, I am sure you will agree that the code like above
is, well, nasty and shouldn't exist.

What about the other non-destructive cases where only reference count is modified without object destruction? In principle,
this is also problematic as the value of the count can be observed. However, this is even less of an issue in practice.
In all intrusive reference counted systems the specific value of reference count is meaningless, can change at any point from
any thread and, in general, developers are always cautioned from even looking at it for non debugging purposes.
Chances of having code somewhere that would do something wrong if the count is decremented inside, rather than outside of a
function are exactly 0. 

So should performance of every smart pointer argument passing be penalized to handle some esoteric condition that never happens in real code? My answer is no, and this is why this library uses the trivial ABI when available.

If an when the standard C++ provides a better solution for wrapper classes this decision can be revisited.
 

