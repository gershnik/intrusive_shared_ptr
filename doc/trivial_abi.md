# A note on clang::trivial_abi attribute

When built with the Clang compiler, `intrusive_shared_ptr` is marked with the [\[\[clang::trivial_abi\]\]](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi) attribute. A good description of what this attribute does and why it is important
for performance can be found [here](https://quuxplusone.github.io/blog/2018/05/02/trivial-abi-101/).
Another take on the performance issue as a comment on a standard library proposal can be found 
[here](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1351r0.html#params).

However, using the trivial ABI with a type that has a non-trivial destructor (and, of course, a smart pointer destructor is non-trivial: it has to decrement the reference count!) immediately raises serious objections. Such use makes the
destructor run *out of order* inside the called function, whereas destructors of other parameters run after the
function exits. While on a theoretical level this is indeed wrong, the interesting question is whether it can ever matter
in practice.

An important observation is that the fact that the destructor runs out of order only really matters in one circumstance: 
when the smart pointer object holds the last reference to the pointee and its destructor causes the pointee to be
destroyed. This out-of-order destruction can be, in principle, observed by outside code. Or can it?
If the smart pointer passed as a function argument holds the last reference, then, for some other code to observe the
pointee's demise, it would need to refer to it via an unsafe raw pointer and only while the smart pointer is alive.
Something like this:

```cpp

T * raw = ...;
struct nasty
{
    nasty(T * p): _p(p) {}
    ~nasty()
    {
        //use _p-> here
    }
    T * _p;
};
foo(intrusive_shared_ptr<T>(noref(raw)), nasty(raw));

```

Assuming **left-to-right order of evaluation** for function arguments, this indeed will do bad things in the `nasty` destructor.
This is indeed what happens with Clang on x64 macOS.

But wait a minute, even though there is a [proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0145r3.pdf) 
to fix the order of function argument evaluation, this is not yet part of the standard, and you cannot rely on a specific
order here. The code is actually broken even without the trivial ABI. And indeed it is on GCC 7.4 on x64 Ubuntu.
Even if the order of evaluation becomes fixed in some future C++ standard, I am sure you will agree that code like the above
is, well, nasty and shouldn't exist.

What about the other non-destructive cases where only the reference count is modified without object destruction? In principle,
this is also problematic as the value of the count can be observed. However, this is even less of an issue in practice.
In all intrusive reference-counted systems, the specific value of the reference count is meaningless, can change at any point from
any thread, and, in general, developers are always cautioned against even looking at it for non-debugging purposes.
The chances of having code somewhere that would do something wrong if the count is decremented inside rather than outside of a
function are exactly 0. 

So should the performance of every smart pointer argument passing be penalized to handle some esoteric condition that never happens in real code? My answer is no, and this is why this library uses the trivial ABI when available.

If and when the C++ standard provides a better solution for wrapper classes, this decision can be revisited.
 

