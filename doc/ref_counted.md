# Header `<intrusive_shared_ptr/ref_counted.h>`

<!-- TOC depthfrom:2 -->

- [Class isptr::ref_counted](#class-isptrref_counted)
    - [Customization](#customization)
    - [Usage](#usage)
    - [Rules for constructors and destructors](#rules-for-constructors-and-destructors)
    - [Limitations](#limitations)
- [Class isptr::weak_reference](#class-isptrweak_reference)
    - [Customization](#customization)
    - [Usage](#usage)
    - [Types](#types)
    - [Methods](#methods)
- [Class isptr::ref_counted_adapter](#class-isptrref_counted_adapter)
    - [Methods](#methods)
- [Class isptr::ref_counted_wrapper](#class-isptrref_counted_wrapper)
    - [Members](#members)
    - [Methods](#methods)

<!-- /TOC -->

## Class isptr::ref_counted

```cpp
template<class Derived, 
         ref_counted_flags Flags = ref_counted_flags::none, 
         class CountType = default_count_type<Flags>>
class ref_counted;

template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
using weak_ref_counted = ref_counted<Derived, Flags | ref_counted_flags::provide_weak_references>;
```

`ref_counted` is meant to be used as base class for any class you want to make reference counted.
It is supposed to be used in conjunction with [`refcnt_ptr`](refcnt_ptr.md) specialization of `intrusive_shared_ptr` but,
of course, can also be used via manual reference counting calls or other smart pointers if desired.

It uses CRTP to access the derived class, avoiding the need for and overhead of virtual functions.
Thus, the first template parameter: `Derived` must be the name of the derived class. Other template
parameters have reasonable defaults and allow customization of reference counting functionality. 
These are described below.


### Customization

`ref_counted` can be customized in the following ways:

* Support for weak references. Default: no. Weak reference support adds some performance overhead even for object which never have weak reference taken so it is not enabled by default. In addition weak reference support requires the reference count type to be `intptr_t` which might be wasteful for many applications where `int` is sufficient.
* Single-threaded mode. Default: no. In single threaded mode reference count manipulations are not thread safe so you cannot
share `refcnt_ptr`s across threads. This results in increased performance though, and might be worthwhile in some scenarios.
* Type of the reference count. Can only be customized if weak references are not supported. Default: `int`. If weak references are supported the count type is always `intptr_t`. You can reduce the size of derived classes by using a smaller type if it can fit the largest expected count. This type must be a signed integral type.
* Many methods of `ref_counted` are accessed via CRTP calls to `Dervied`. This allows "overriding" them in your derived class to modify or augment their functionality.
  * In particular, `destroy()` member function is called to actually destroy the instance when the reference count drops to 0. The base `ref_counted` implementation invokes `delete` on `Derived` pointer. Overriding this function in derived class allows you to handle objects allocated in a different way.

The template parameter `Flags` is a set of flags of type `ref_counted_flags`. These can be bitwise OR-ed to combine.
By default no flags are set. Currently only two flags are defined:
* `ref_counted_flags::provide_weak_references` - enables weak references
* `ref_counted_flags::single_threaded` - enables single threaded mode
More flags may be added in the future.

The template parameter CountType can be specified to indicate the type of reference count (if `ref_counted_flags::provide_weak_references` is not set). It must be a signed integral type.

For convenience the library provides the following typedefs:

- `template<class Derived> weak_ref_counted` is a `ref_counted` with `ref_counted_flags::provide_weak_references` flag.
- `template<class Derived, class CountType=...> ref_counted_st` is `ref_counted` with `ref_counted_flags::single_threaed` flag.
- `template<class Derived> weak_ref_counted_st` is a `ref_counted` with both `provide_weak_references` and `single_threaed` 
flags set.


### Usage

You use `ref_counted` by deriving from it and making it a friend.
You probably want to make your destructor private or protected (if your class is itself a base). Reference counted object should only be destroyed via reference counting, not via direct `delete` calls.


Example:

```cpp
class foo : public ref_counted<foo>
{
friend ref_counted;
private:
    ~foo() noexcept = default;
};
```

### Rules for constructors and destructors

In your class derived from `ref_counted` you must follow these rules:

- **Never increment reference count (by creating `refcnt_ptr` from `this` for example or any other means) in destructor.** Doing so will trigger an assert in debug builds and cause undefined behavior in release ones.
  - It is generally ok to create weak references to `this` in destructor.
- Never give out `this` (by creating `refcnt_ptr` from `this` for example or any other means) or a weak reference to `this` in a constructor **if any subsequent code in constructor can exit via an exception**. If such exception is thrown your object will fail to be created leaving the outside code holding an invalid pointer. This is a general C++ rule. Usage of reference counting does not negate it.
  - It is ok and supported to do reference counting in constructor **if any subsequent code in constructor does not exit via exception**
- Your destructor must not throw exceptions. Another general C++ rule made even more relevant here, since the reference counting machinery in `ref_counted` does not expect exception to be thrown and will break in this case.
- If your class *is* a base then the destructor needs to be virtual - `ref_counted` will invoke destructor of `Derived` template argument. (If you propagate most derived class as `Derived` argument then this obviously does not apply).


### Limitations

When not using weak references the implementation of `ref_counted` is standard compliant and portable (modulo bugs and compiler deficiencies of course). The only practical limitation is that total number of outstanding references to an object must be between 0 and `std::numeric_limits<CountType>::max()`.

When using weak references `ref_counted` relies on multiplexing pointers and reference count in the same memory location. This mechanism should work provided:
* The platform uses 2s complement arithmetic 
* `alignof(intptr_t) > 1`
* When cast to `uintptr_t` the value of a pointer to an object with alignment bigger than 1 has the lowest bit 0 (e.g. is odd).

All these conditions seem to be true for all platforms these days but who knows...

```

### Types

`ref_counted` declares the following public types:

* `ref_counted_base` - a synonym for itself. This allows to refer to the type from its derived types without caring about specific template arguments like `derived::ref_counted_base`
* `weak_value_type` - if weak references are enabled this is the type of weak reference object this class exposes. Otherwise `void`
* `weak_ptr` - if weak references are enabled a synonym for `refcnt_ptr<weak_value_type>`. Otherwise `void`
* `const_weak_ptr` - if weak references are enabled a synonym for `refcnt_ptr<const weak_value_type>`. Otherwise `void`.

### Constants

* `static constexpr bool provides_weak_references` - `true` if this class provides weak references
* `static constexpr bool single_threaded` - `true` if this class is single threaded

### Methods

Unless specified otherwise all methods of this class are `noexcept`.

* Copy and move constructors are **deleted**. This is a base class and slicing is meaningless.
* Copy and move assignment operators are similarly **deleted**
* Protected default constructor. Reference count is set to 1 in it.
* Protected destructor. 
* Protected `void destroy() const noexcept`. Called when reference count drops to 0 to destroy the object. The default implementation calls `delete` on derived class pointer. Can be overridden in a derived class.
* Public `void add_ref() const noexcept`. Increments reference count. Can be overridden in a derived class.
* Public `void sub_ref() const noexcept`. Decrements reference count and destroys the object if it reaches 0. Can be overridden in a derived class.
* If the class supports weak references then two additional public methods are available <br/>
  `weak_ptr get_weak_ptr()` and <br/>
  `const_weak_ptr get_weak_ptr() const`. <br/>
  These methods are **not** `noexcept`. Weak reference "control block" is created lazily when a weak reference is requested for the first time. If memory allocation or customized weak reference class constructor throws these methods will throw. Subsequent calls will be `noexcept`. 
* Protected `const weak_value_type * get_weak_value() const`. Only meaningful if the class supports weak references. This is the actual method that retrieves raw pointer to weak reference used to implement `get_weak_ptr`. Not `nonexcept`. Can be overridden in a derived class.
* Protected `weak_value_type * make_weak_reference(intptr_t count) const`. Only meaningful if the class supports weak references. This method is called to create weak reference (control block) when one is needed for the first time. The returned pointer has its own reference count already incremented (as appropriate for a method returning raw pointer). Can be overridden in a derived class.


### Customizing weak reference type 

It is possible to customize weak reference type used by your class by "overriding" `make_weak_reference`.
`ref_counted` looks via CRTP for a function with the following signature in your derived class:
```cpp
some_type * make_weak_reference(intptr_t count) const
```
where some_type must be a class derived from `isptr::weak_reference` (see below).
If such function exists it will be called instead of the one provided by `ref_counted` itself and the type it returns will be the weak reference type.

## Class isptr::weak_reference

```cpp
template<class Owner>
class weak_reference;
```

`weak_reference` represents a weak reference to a class derived from `ref_counted`.
It usually is used as-is but can be used as a base class for customized weak references.

`Owner` template parameter is the actual class the weak reference is a reference to. 

Internally `weak_reference` is a "control block" for a `ref_counted`. It is reference counted itself and also manages the count for the referenced object. `weak_reference` objects are created on-demand when a first weak reference is requested from `ref_counted`

### Customization

You can derive your own class from `weak_reference` and use it in conjunction with your class derived from `ref_counted`. See [Customizing weak reference type](#customizing-weak-reference-type) for details.
Many methods of `weak_reference` are accessed via CRTP calls to the derived class. This allows "overriding" them in your derived class to modify or augment their functionality.
In particular, `destroy()` member function is called to actually destroy the instance when the reference count drops to 0. The base `weak_reference` implementation invokes `delete` on your class pointer. Overriding this function in derived class allows you to handle objects allocated in a different way.


### Usage

Usually there is no need to explicitly mention `weak_reference` in your code. The type is accessible as `weak_value_type` typedef of your `ref_counted`-derived class. If you use `refcnt_ptr` you can convert between weak and strong pointers in the following fashion:

```cpp
auto original = refcnt_attach(new your_object());

auto weak = weak_cast(original);
//or
your_object::weak_ptr weak = weak_cast(original);

auto strong = strong_cast(weak);
assert(strong == original || strong == nullptr);
```

You can also use member functions instead of `weak_cast`/`strong_cast`

```cpp
auto original = refcnt_attach(new your_object());

auto weak = original->get_weak_ptr();
//or
your_object::weak_ptr weak = original->get_weak_ptr();

auto strong = weak->lock();
assert(strong == original || strong == nullptr);
```

Note that `const`-ness propagates between strong and weak pointers. A strong pointer to const yields weak pointer to const and vice versa. 


### Types

`weak_reference` declares the following public types

* `strong_value_type` - a synonym for `Owner`
* `strong_ptr` - a synonym for `refcnt_ptr<strong_value_type>`
* `const_strong_ptr` - a synonym for `refcnt_ptr<const strong_value_type>`

### Methods

Unless specified otherwise all methods of this class are `noexcept`.

* Copy and move constructors are **deleted**. Copying of weak references is meaningless.
* Copy and move assignment operators are similarly **deleted**
* Protected constructor `constexpr weak_reference(intptr_t initial_strong, Owner * owner) noexcept`
  The `initial_strong` parameter is the initial value for the `Owner`s reference count. Since `weak_reference` objects are created on-demand the referent's count can be any value that was reached prior to `weak_reference` creation.
  The `owner` is a raw pointer to the `Owner` object.
* Protected destructor. 
* Protected `void destroy() const noexcept`. Called when object's own reference count drops to 0 to destroy the object. The default implementation calls `delete` on derived class pointer. Can be overridden in a derived class.
* Public `void add_ref() const noexcept`. Increments object's own reference count. Can be overridden in a derived class.
* Public `void sub_ref() const noexcept`. Decrements object's own reference count and destroys the object if it reaches 0. Note that the `Owner` always holds a reference to its `weak_reference` (control block) so it will only be destroyed after the `Owner` object. Can be overridden in a derived class.
* Public <br/>
  `const_strong_ptr lock() const noexcept` and <br/>
  `strong_ptr lock() noexcept` <br/>
  Obtain a strong reference to `Owner`. The return value is a `null` smart pointer if the owner no longer exists.
* Protected <br/>
  `void add_owner_ref() noexcept` and <br/>
  `void sub_owner_ref() noexcept` <br/>
  These manage reference count of the `Owner`. Can be overridden in a derived class.
* Protected `strong_value_type * lock_owner() const noexcept`
  This method does the actual locking and returns a pointer to owner (with reference count incremented) or `null`. Can be overridden in a derived class.
* Protected `void on_owner_destruction() const noexcept`. 
  This method does nothing. It can be re-declared in a derived class. It is invoked *after* the `Owner` object has been destroyed - that is when the number of strong references to it becomes 0. This provides a customization point for derived classes in case they need to clean up some data associated with the owner. Note that the method is called after owner's destruction so you cannot access or resurrect owner from it.
 


## Class isptr::ref_counted_adapter

```cpp
template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
class ref_counted_adapter;

template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
using weak_ref_counted_adapter = ref_counted_adapter<Derived, Flags | ref_counted_flags::provide_weak_references>;

```

This class publicly derives from a non-reference counted class `T` and a `ref_counted`. The rest template parameters are forwarded to `ref_counted`.

### Methods

* Public constructor. Perfectly forwards to constructor of `T`. `noexcept` if `T`'s constructor is.
* Protected destructor. 

## Class isptr::ref_counted_wrapper

```cpp
template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
class ref_counted_wrapper;

template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
using weak_ref_counted_wrapper = ref_counted_wrapper<Derived, Flags | ref_counted_flags::provide_weak_references>;

```

This class stores (wraps) a member of class `T` and derives form `ref_counted`. The rest template parameters are forwarded to `ref_counted`.

### Members

* Public `T wrapped`. The wrapped instance of `T`. 

### Methods

* Public constructor. Perfectly forwards to constructor of `wrapped`. `noexcept` if `T`'s constructor is.
* Protected destructor. 

