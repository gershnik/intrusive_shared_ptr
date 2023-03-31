# A note on implementing reference counted objects

Making an object reference counted can be tricky. There are some fundamental problems, tricky situations and some design choices that need to be taken care of.

The basics are pretty well known. People usually create a class along the following lines

```cpp
class foo
{
public:
    void add_ref() noexcept
       { ++m_count; }
    void sub_ref() noexcept
    {
        if (--m_count == 0)
            delete this;
    }
private:
    atomic<int> m_count;
};
```

So far so good but why add_ref and sub_ref are not const? A reference count is really not a part
of the object state. Consider that you might want to have `smart_ptr<const T>` in your code and
such smart pointer will have to call these methods on a const object. So here is the first revision

```cpp
class foo
{
public:
    void add_ref() const noexcept
       { ++m_count; }
    void sub_ref() const noexcept
    {
        if (--m_count == 0)
            delete this;
    }
private:
    mutable atomic<int> m_count;
};
```

Now the first fundamental problem. What should the reference count be initialized to in object constructor: 
1 or 0? People who use flawed smart pointers with a simple constructor `smart_ptr(T * p)` that always 
bumps the count tend to like 0. This way the object gets the desired one as soon as you stuff it into a smart_ptr.

Unfortunately, this turns out to be a bad idea from two different angles. 
One is performance. Having to always increment count right after construction is a performance penalty. A small
penalty but penalty nonetheless. Starting from 1 avoids it.
The second problem is that while in the body of the constructor you have an object with a 0 reference count. 
Now, the body of the constructor is a tricky place. Consider what happens if you create a smart pointer out of `this` 
within the constructor. This can happen if you call a function that expects a smart pointer parameter 
(e.g. `register_callback(smart_ptr(this))`) and the function does not store the pointer. In this case the count 
is bumped to 1 when the pointer is created but then goes to 0 when it is destroyed. And this
invokes the destructor from inside the constructor. Bang! You've got a nasty piece of undefined behavior happening.
But wait, is creating a smart pointer from `this` in constructor a good idea? Suppose you have the following situation:

```cpp
foo::foo():
    base_class(...)
{
    register_callback(smart_ptr(this));
    
    function_that_may_throw_exception();

    //other initialization
}
```

and `function_that_may_throw_exception` actually throws. In this case external code possesses a pointer to an 
object that failed to construct. Its base classes are destroyed and it is dead from C++ point of view. Nevertheless,
the external code will have a live pointer to it and, worse, invoke destructor when reference count goes to 0.
Note that there is nothing special about this situation - the same bad effect can be achieved with raw pointers if we
give `this` away before an exception is thrown. However, using smart pointers can provide false sense of security here.
The general rule for any C++ class is:

Do not give `this` away to be stored for later use if the constructor can throw after you do so.

Assuming you follow this rule giving this away can be legal and legitimate - but only if you start counting from 1.
To conclude, starting from 1 is faster and allows some techniques that starting from 0 doesn't.
With this is mind here is a revised foo

```cpp
class foo
{
public:
    void add_ref() const noexcept
       { ++m_count; }
    void sub_ref() const noexcept
    {
        if (--m_count == 0)
            delete this;
    }
private:
    mutable atomic<int> m_count = 1;
};
```

Note that having this convention requires you to use
```cpp
intrusive_shared_ptr<foo, Traits> p = intrusive_shared_ptr<foo, Traits>::noref(new foo);
```
to **attach** newly created object rather than bump its count.
	
But we are not off the hook yet. Now consider what happens in destructor. When we enter it, the reference
count is already 0 but what if call some function there too and pass it a smart pointer created from `this`? 
The count will be bumped again then go to 0 and you will have `delete this` running the second time. 

Perhaps, the solution is again to make the count 1? If this is the case for constructor perhaps it makes sense
for destructor too? This would also be a bad idea. First of all bumping the count for all objects even if they
don't care, is, again, a performance penalty. 
Second consider what does it mean to add a reference to an object that is undergoing destruction. Conceivably the code
that added the reference can then store the pointer expecting the object to be safely alive. Then it can try to 
use it later but the object has been already destroyed. Once you think more about it you realize that this is 
the famous  "finalize resurrection" problem (https://en.wikipedia.org/wiki/Object_resurrection) in another form. 
Unfortunately, or rather fortunately, C++ doesn't allow you to "abandon" or "postpone" destruction. Once the 
destructor has been entered the object will become dead. 

It is possible to re-invent the whole notion of finalizer (a separate function called before the destructor) and
resurrection for reference counting but doing so is a lot of work and doing it correctly is very tricky. The
experience with finalizers in other languages is not encouraging.

Instead, consider why would you ever need to give out a reference to an object from a destructor. Constructor 
case is obvious: you might want to register for some callback or notification from somewhere else. You might think
that in a destructor it would be the opposite: deregister the object, but this is wrong. The very fact that you 
*are* in a destructor means that no place else in the code has any knowledge of the object. There is nowhere to
deregister or disconnect from. (It is certainly possible that other places have *weak* references to the object, but
those do not concern us here - there is no issue in creating a *weak* pointer to `this` in destructor and deregistering 
*that*.)

With this in mind, the only sane approach seems to be to disallow adding a reference to an object that is being 
destroyed, period. Any attempt to do so likely indicates a design or implementation mistake. You could detect this
situation (bumping the count from 0) and call `terminate()` or, do it in debug mode only via `assert`.


```cpp
class foo
{
public:
    void add_ref() const noexcept
    {   
        [[maybe_unused]] auto value = ++m_count;
        assert(value > 1);
    }
    void sub_ref() const noexcept
    {
        if (--m_count == 0)
            delete this;
    }
private:
    mutable atomic<int> m_count = 1;
};
```

Now this is the bare minimum for a reference counted class but there are a few more things to take care of.
First the destructor declaration. If this class is going to be a base class for a class hierarchy then the 
destructor must be virtual and protected (to avoid manual deletion not done via reference counting).

```cpp
class foo
{
    //... as above
protected:
    virtual ~foo() noexcept = default;
};
```

(If you use CRTP you can avoid `virtual` here - this is beyond the scope of this article)

Alternatively if the class is standalone it should be final with a private non-virtual destructor.

```cpp
class foo final
{
    //... as above
private:
    ~foo() noexcept = default;
};
```

The reference counting itself can also be made more efficient. Operators `++` and `--` perform a full fence whereas
we don't really need it. A better approach is

```cpp
class foo
{
public:
    void add_ref() const noexcept
    { 
        [[maybe_unused]] auto old_value = m_count.fetch_add(1, std::memory_order_relaxed); 
        assert(old_value > 0);
    }
    void sub_ref() const noexcept
    {
        if (m_count.fetch_sub(1, std::memory_order_release) == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }
protected:
    virtual ~foo() noexcept = default;
private:
    mutable atomic<int> m_count = 1;
};
```
