# Intro 
This is yet another implementation of an intrusive [reference counting](http://en.wikipedia.org/wiki/Reference_counting) 
[smart pointer](http://en.wikipedia.org/wiki/Smart_pointer), highly configurable reference counted base class and adapters.
The code requires C++17 or above compiler.
It is known to work with:<br/>
Xcode 11 or above<br/>
Microsoft Visual Studio 2019 or above<br/>
GCC 7.4.0 or above<br/>

Documentation and formal tests are work in progress.

<!-- TOC depthfrom:2 -->

- [Why bother?](#why-bother)
    - [Named conversions from raw pointers](#named-conversions-from-raw-pointers)
    - [No ADL](#no-adl)
    - [Decent support for output parameters](#decent-support-for-output-parameters)
    - [Support for operator->*](#support-for-operator-)
    - [Atomic access](#atomic-access)
    - [Trivial ABI](#trivial-abi)
    - [Correct implementation of a "reference counted base" class](#correct-implementation-of-a-reference-counted-base-class)
    - [Support for weak pointers](#support-for-weak-pointers)
- [Integration](#integration)
    - [CMake via FetchContent](#cmake-via-fetchcontent)
    - [Building and installing on your system](#building-and-installing-on-your-system)
        - [Basic use](#basic-use)
        - [CMake package](#cmake-package)
        - [Via pkg-config](#via-pkg-config)
    - [Copying to your sources](#copying-to-your-sources)
- [Usage](#usage)
    - [Using provided base classes](#using-provided-base-classes)
    - [Supporting weak pointers](#supporting-weak-pointers)
    - [Using with Apple CoreFoundation types](#using-with-apple-corefoundation-types)
    - [Using with Microsoft COM interfaces](#using-with-microsoft-com-interfaces)
    - [Using with Python objects](#using-with-python-objects)
    - [Using with non-reference counted types](#using-with-non-reference-counted-types)
    - [Atomic operations](#atomic-operations)
- [Constexpr functionality](#constexpr-functionality)
- [Reference](#reference)

<!-- /TOC -->

## Why bother?
There are multiple other intrusive smart pointers available including one from [Boost](https://www.boost.org/doc/libs/1_71_0/libs/smart_ptr/doc/html/smart_ptr.html#intrusive_ptr)
and nowadays there is even a [proposal](http://open-std.org/JTC1/SC22/WG21/docs/papers/2016/p0468r0.html) 
to add one to standard library C++ 2023, so why create another one?
Unfortunately, as far as I can tell, all existing implementations, and that includes the standard library proposal at the time
of this writing, suffer from numerous deficiencies that make them hard or annoying to use in real life code. 
The most serious problems addressed here are as follows

### Named conversions from raw pointers
All other libraries offer a conversion in the form 
`smart_ptr(T * p);`
In my opinions this is an extremely bad design. When looking at a call like `smart_ptr(foo())` can you quickly tell whether this adds a reference count or "attaches" the smart pointer to a raw one? That's right, you cannot! The answer depends 
on the smart pointer implementation or even on specific traits used. This makes it invisible and hard to predict at the **call site** and 100% guarantees that someone will make a wrong assumption. In my experience, almost all reference counting bugs happen on the **boundary** between C++ and C code where such conversions are abundant. 
Just like any form of dangerous cast this one has to be **explicit** in calling code (as an aside, ObjectiveC ARC did it right
with their explicit and visible `__bridge` casts between raw and smart pointers).
Note that having a boolean argument (like what Boost and many other implementations do) in constructor isn't a solution.
Can you quickly tell what `smart_ptr(p, true)` does? Is it "true, add reference" or "true, copy it"?

This library uses named functions to perform conversion. You see exactly what is being done at the call site.

### No ADL 

Many libraries use [ADL](https://en.wikipedia.org/wiki/Argument-dependent_name_lookup) to find "add reference" and 
"release reference" functions for the underlying type.
That is they have expressions like `add_ref(p)` in their implementation, and expect a function named `add_ref` that accepts pointer to the underlying type is supposed to be found via ADL.
This solution is great in many cases but it breaks when working with some C types like Apple's `CTypeRef`. This one is actually a typedef to `void *` so if you have an `add_ref` that accepts it, you have just made every unrelated `void *` reference counted (with very bad results if you accidentally put a wrong one into a smart pointer).
A better approach to defining how reference counting is done is to pass a traits class to the smart pointer. (The standard library proposal gets this one right).

This library uses traits

### Decent support for output parameters

Often times you need to pass smart pointer as an output parameter to a C function that takes `T **`
Many other smart pointers either 
- ignore this scenario, requiring you to introduce extra raw pointer and unsafe code, or
- overload `operator&` which is a horrendously bad idea (it breaks lots of generic code which assumes that `&foo` gives
  an address of foo, not something else)
The right solution is to have a proxy class convertible to `T **`.
The standard library proposal addresses this problem via generic `out_ptr` that can work with any 
smart pointer. If done right, this might be the best solution but the relevant code is not yet widely available anywhere.

This library currently uses an inner proxy class and a `get_output_param()` method. 

### Support for `operator->*`

This might seem to be a minor thing but is really annoying in generic code. For some reason no smart pointers bother to provide
`operator->*` so that pointers to members could be accessed via the same syntax as for raw pointers. In non-generic code
you can always work around it via `(*p).*whatever` but in generic code this is not an option.

### Atomic access

Sometimes you need to operate on smart pointers atomically. To the best of my knowledge no library currently provides this functionality.

This library provides a specialization of `std::atomic<intrusive_shared_ptr<...>>` extending to it the normal `std::atomic` semantics.

### Trivial ABI

When built with CLang compiler `intrusive_shared_ptr` is marked with [\[\[clang::trivial_abi\]\]](https://clang.llvm.org/docs/AttributeReference.html#trivial-abi) attribute. A good description of what this attribute does and why it is important
for performance can be found [here](https://quuxplusone.github.io/blog/2018/05/02/trivial-abi-101/).
Another take on the performance issue as a comment on standard library proposal can be found 
[here](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1351r0.html#params).
[This page](doc/trivial_abi.md) contains details on why this is a good idea and why concerns about order of destruction do not really matter here.

### Correct implementation of a "reference counted base" class

This is not directly a problem with smart pointers but with a base class often provided together with them to implement an
intrusively counted class. Very often they contain subtle bugs (see 
['A note on implementing reference counted objects'](doc/reference_counting.md) for more details). It is also tricky to 
create a base class that can work well for different requirements without compromising efficiency.

### Support for weak pointers

Continuing on the base class theme, when doing intrusive reference counting, supporting (or not) weak pointers is the responsibility of the counted class. Supporting weak pointers also usually involves tradeoffs in terms of performance or memory consumption. 
This library allows to enable a decent implementation of weak pointers via policy based design. 


## Integration

### CMake via FetchContent

```cmake
include(FetchContent)
...
FetchContent_Declare(isptr
    GIT_REPOSITORY  https://github.com/gershnik/intrusive_shared_ptr.git
    GIT_TAG         v1.4  #use the tag, branch or sha you need
    GIT_SHALLOW     TRUE
)
...
FetchContent_MakeAvailable(isptr)
...
target_link_libraries(mytarget
PRIVATE
  isptr::isptr
)
```
> ℹ&#xFE0F; _[What is FetchContent?](https://cmake.org/cmake/help/latest/module/FetchContent.html)_

### Building and installing on your system

You can also build and install this library on your system using CMake.

1. Download or clone this repository into SOME_PATH
2. On command line:
```bash
cd SOME_PATH
cmake -S . -B build 
cmake --build build

#Optional
#cmake --build build --target run-test

#install to /usr/local
sudo cmake --install build
#or for a different prefix
#cmake --install build --prefix /usr
```

Once the library has been installed it can be used int the following ways:

#### Basic use 

Set the include directory to `<prefix>/include` where `<prefix>` is the install prefix from above.

#### CMake package

```cmake
find_package(isptr)

target_link_libraries(mytarget
PRIVATE
  isptr::isptr
)
```

#### Via `pkg-config`

Add the output of `pkg-config --cflags isptr` to your compiler flags.

Note that the default installation prefix `/usr/local` might not be in the list of places your
`pkg-config` looks into. If so you might need to do:
```bash
export PKG_CONFIG_PATH=/usr/local/share/pkgconfig
```
before running `pkg-config`


### Copying to your sources

You can also simply download the headers of this repository from [Releases](https://github.com/gershnik/intrusive_shared_ptr/releases) page 
(named `intrusive_shared_ptr-X.Y.tar.gz`), unpack it somewhere in your source tree and add it to your include path.


## Usage 

All the types in this library are declared in `namespace isptr`. For brevity the namespace is omitted below.
Add `isptr::` prefix to all the type or use `using` declaration in your own code.

The header `<intrusive_shared_ptr/intrusive_shared_ptr.h>` provides a template

```cpp
template<class T, class Traits>
class intrusive_shared_ptr<T, Traits>;
```

Where `T` is the type of the pointee and `Traits` a class that should provide 2 static functions that look like this

```cpp
static void add_ref(SomeType * ptr) noexcept
{  
    //increment reference count. ptr is guaranteed to be non-nullptr
}
static void sub_ref(SomeType * ptr) noexcept
{ 
    //decrement reference count. ptr is guaranteed to be non-nullptr
}
```

`SomeType *` should be a pointer type to which `T *` is convertible to. It is possible to make `add_ref` and `sub_ref`
templates, if desired, though this is usually not necessary.


To create `intrusive_shared_ptr` from a raw `T *` there are 2 functions:

```cpp
//pass the smart pointer in without changing the reference count
template<class T, class Traits>
intrusive_shared_ptr<T, Traits> intrusive_shared_ptr<T, Traits>::noref(T * p) noexcept;

//adopt the pointer and bump the reference count
template<class T, class Traits>
intrusive_shared_ptr<T, Traits> intrusive_shared_ptr<T, Traits>::ref(T * p) noexcept
```

It is possible to use `intrusive_shared_ptr` directly but the name is long and ugly so a better approach is to
wrap in a typedef and wrapper functions like this

```cpp
struct my_type
{};

struct my_intrusive_traits
{
    static void add_ref(my_type * ptr) noexcept; //implement
    static void sub_ref(my_type * ptr) noexcept; //implement
};

template<class T>
using my_ptr = intrusive_shared_ptr<T, my_intrusive_traits>;

template<class T> 
my_ptr<T> my_retain_func(T * ptr) {
    return my_ptr<T>::ref(ptr);
}
template<class T> 
my_ptr<T> my_attach_func(T * ptr) {
    return my_ptr<T>::noref(ptr);
}

```

The library provides such wrappers for some common scenarios. If you fully control the definition of `my_type` then
it is possible to simplify things even further with header `refcnt_ptr.h`. It adapts `intrusive_shared_ptr` to traits
exposed as inner type `refcnt_ptr_traits`. You can use it like this:

```cpp
#include <intrusive_shared_ptr/refcnt_ptr.h>

struct my_type
{
  struct refcnt_ptr_traits
  {
      static void add_ref(my_type * ptr) noexcept; //implement
      static void sub_ref(my_type * ptr) noexcept; //implement
  };
};

//now you can use refcnt_ptr<my_type> for the pointer type and refcnt_attach and refcnt_retain free functions e.g.

//create from raw pointer (created with count 1)
foo raw = new my_type();
refcnt_ptr<my_type> p1 = refcnt_attach(raw);

//create directly
auto p1 = make_refcnt<my_type>();

//assign from raw pointer bumping reference count
refcnt_ptr<my_type> p2;
p2 = refcnt_retain(raw);


```


### Using provided base classes

To implement `my_type` above the library provides a base class you can inherit from which will do the right thing.

```cpp
#include <intrusive_shared_ptr/ref_counted.h>
#include <intrusive_shared_ptr/refcnt_ptr.h>

class foo : ref_counted<foo>
{
    friend ref_counted;
public:    
    void method();
private:
    ~foo() noexcept = default; //prevent manual deletion
};

//you can use auto to declare p1, p2 and p3. The full type is spelled out for
//demonstration purposes only

//attach from raw pointer (created with count 1)
refcnt_ptr<foo> p1 = refcnt_attach(new foo()); 

//create directly
refcnt_ptr<foo> p2 = make_refcnt<foo>();

//assign from raw pointer bumping reference count
foo * raw = ...
refcnt_ptr<foo> p3 = refcnt_retain(raw);

```

The type of the reference count is `int` by default. If you need to you can customize it. 

```cpp

class tiny : ref_counted<tiny, ref_counted_flags::none, char> //use char as count type
{
  friend ref_counted;

  char c;
};

static_assert(sizeof(tiny) == 2);

```

More details can be found in [this document](doc/ref_counted.md)

### Supporting weak pointers

If you want to support weak pointers you need to tell `ref_counted` about it. Since weak pointers include overhead
even if you never create one by default they are disabled.

```cpp
#include <intrusive_shared_ptr/ref_counted.h>
#include <intrusive_shared_ptr/refcnt_ptr.h>

class foo : weak_ref_counted<foo> //alias for ref_counted<foo, ref_counted_flags::provide_weak_references>
{
    void method();
};

refcnt_ptr<foo> p1 = refcnt_attach(new foo());
foo::weak_ptr w1 = p1->get_weak_ptr();
refcnt_ptr<foo> p2 = w1->lock();
```

Note that you cannot customize the type of reference count if you support weak pointers - it will always be `intptr_t`.
More details can be found in [this document](doc/ref_counted.md)

### Using with Apple CoreFoundation types

```cpp

#include <intrusive_shared_ptr/apple_cf_ptr.h>

//Use auto in real code. Type is spelled out for clarity
cf_ptr<CStringRef> str = cf_attach(CFStringCreateWithCString(nullptr, "Hello", kCFStringEncodingUTF8));
std::cout << CFStringGetLength(str.get());

CFArrayRef raw = ...;
//Use auto in real code.
cf_ptr<CFArrayRef> array = cf_retain(raw);

```

### Using with Microsoft COM interfaces

```cpp

#include <intrusive_shared_ptr/com_ptr.h>

com_shared_ptr<IStream> pStream;
CreateStreamOnHGlobal(nullptr, true, pStream.get_output_param());
pStream->Write(....);

```

### Using with Python objects

```cpp
#include <intrusive_shared_ptr/python_ptr.h>

auto str = py_attach(PyUnicode_FromString("Hello"));
std::cout << PyUnicode_GetLength(str.get());

```

### Using with non-reference counted types

On occasion when you have a code that uses intrusive reference counting a lot you might need to handle a type
which you cannot modify and which is not by itself reference counted. 
In such situation you can use an adapter (if you prefer derivation) or wrapper (if you prefer containment) that makes it such

Adapter:
```cpp
#include <intrusive_shared_ptr/ref_counted.h>

using counted_map = ref_counted_adapter<std::map<string, int>>;

auto ptr = make_refcnt<counted_map>();
(*ptr)["abc"] = 7;
std::cout << ptr->size();

using weakly_counted_map = weak_ref_counted_adapter<std::map<string, int>>;

auto ptr1 = make_refcnt<weakly_counted_map>();
(*ptr1)["abc"] = 7;
std::cout << ptr1->size();
foo::weak_ptr w1 = p1->get_weak_ptr();
refcnt_ptr<weakly_counted_map> p2 = w1->lock();
```

Wrapper:
```cpp
#include <intrusive_shared_ptr/ref_counted.h>

using counted_map = ref_counted_wrapper<std::map<string, int>>;

auto ptr = make_refcnt<counted_map>();
ptr->wrapped()["abc"] = 7;
std::cout << ptr->wrapped().size();

using weakly_counted_map = weak_ref_counted_wrapper<std::map<string, int>>;

auto ptr1 = make_refcnt<weakly_counted_map>();
ptr1->wrapped()["abc"] = 7;
std::cout << ptr1->wrapped().size();
foo::weak_ptr w1 = p1->get_weak_ptr();
refcnt_ptr<weakly_counted_map> p2 = w1->lock();
```

### Atomic operations

The library provides a partial specialization 

```cpp

template <class T, class Traits>
std::atomic<intrusive_shared_ptr<T, Traits>>;

```

which exposes normal `std::atomic` functionality. For example:

```cpp

using my_ptr = intrusive_shared_ptr<my_type, my_intrusive_traits>;

using my_atomic_ptr = std::atomic<my_ptr>;

my_ptr ptr = ...;
my_atomic_ptr aptr = ptr;

ptr = aptr.load();
//or
ptr = aptr;

aptr.store(ptr);
//or
aptr = ptr;

my_ptr ptr1 = aptr.exchange(ptr);

//etc.

```

## Constexpr functionality

When built with C++20 compiler `intrusive_shared_ptr` is fully constexpr capable. You can do things like

```cpp

using my_ptr = intrusive_shared_ptr<my_type, my_intrusive_traits>;

constexpr my_ptr foo;

```

Due to non-default destructors this functionality is not available on C++17

## Reference

* [intrusive_shared_ptr.h](doc/intrusive_shared_ptr.md)
* [refcnt_ptr.h](doc/refcnt_ptr.md)
* [ref_counted.h](doc/ref_counted.md)



