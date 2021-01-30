# Header `<intrusive_shared_ptr/ref_counted.h>`

## Class isptr::ref_counted

```cpp
template<class Derived, 
         ref_counted_flags Flags = ref_counted_flags::none, 
         class CountType = default_count_type<Flags>>
class ref_counted;
```

`ref_counted` is meant to be used as base class for any class you want to make reference counted.
It is supposed to be used in conjuction with `refcnt_ptr` specialization of `intrusive_shared_ptr` but,
of course, can be used via manual reference counting calls or other smart pointers if desired.

It uses CRTP to access the derived class, avoiding the need for and overhead of virtual functions.
Thus, the first template parameter: `Derived` must be the name of the derived class. Other template
parameters have reasonable defaults and allow customization of reference counting functionality. 
These are described below.


### Customization

`ref_counted` can be customized in the following ways:

* Support for weak references. Default: no. Weak reference support adds some performance overhead even for object which never have weak reference taken so it is not enabled by default. In addition weak reference support requires the reference count type to be `intptr_t` which might be wasteful for many applications where `int` is sufficient.
* Type of the reference count. Can only be customized if weak references are not supported. Default: `int`. If weak references are supported the count type is always `intptr_t`. You can reduce the size of derived classes by using a smaller type if it can fit the largest expected count. This type must be a signed integral type.
* Whether to use an abstract base class as base of `ref_counted` (and weak reference if supported). Abstract base class allows treating all counted object polymorphically (e.g. have a container of `refcnt_ptr<abstract_ref_counted>`). It is generally slower (due to overhead of virtual calls) than the default approach but might be more convenient in some cases.

The template parameter `Flags` is a set of flags of type `ref_counted_flags`. These can be bitwise OR-ed to combine.
By default no flags are set. You can specify any combination of the following:
`ref_counted_flags::provide_weak_references` - enables weak references
`ref_counted_flags::use_abstract_base` - use abstract base classes

The template parameter CountType can be specified to indicate the type of reference count (if `ref_counted_flags::provide_weak_references` is not set). It must be a signed integral type.

For the common case where you just want weak references there is a convenience typedef 

```cpp
template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
using weak_ref_counted = ref_counted<Derived, Flags | ref_counted_flags::provide_weak_references>;
```

which adds sets `ref_counted_flags::provide_weak_references` and disallows setting the count type.

### Usage

You use `ref_counted` by deriving from it. If you do not use abstract base classes then you also need to make `ref_counted` a friend.
In either case you probably want to make the destructor private (if your class is itself not a base) or protected. Reference counted object should only be destroyed via reference counting, not via direct `delete` calls.

Example:

```cpp
class foo : public ref_counted<foo>
{
friend ref_counted;
private:
    ~foo() noexcept = default;
};

or

class foo : public ref_counted<foo, ref_counted_flags::use_abstract_base>
{
protected:
    ~foo() noexcept override = default;
};

```

### Rules for constructors and destructors

In your class derived from `ref_counted` you must follow these rules:

- **Never increment reference count (by creating `refcnt_ptr` from `this` for example or any other means) in destructor.** Doing so will trigger an assert in debug builds and cause undefined behavior in release ones.
  - It is generally ok to create weak references to `this` in destructor.
- Never give out `this` (by creating `refcnt_ptr` from `this` for example or any other means) or a weak reference to `this` in a constructor **if any subsequent code in constructor can exit via an exception**. If such exception is thrown your object will fail to be created leaving the outside code holding an invalid pointer. This is a general C++ rule. Usage of reference counting does not negate it.
  - It is ok and supported to do reference counting in constructor **if any subsequent code in constructor does not exit via exception**
- Your destructor must not throw exceptions. Another general C++ rule made even more relevant here, since the reference counting machinery in `ref_counted` does not expect exception to be thrown and will break in this case.

### Limitations

When not using weak references the implementation of `ref_counted` is standard compliant and portable (modulo bugs and compiler deficiencies of course). The only practical limitation is that total number of oustanding references to an object must be between 0 and `std::numeric_limits<CountType>::max()`.

When using weak references `ref_counted` relies on multiplexing pointers and reference count in the same memory location. This should work provided:
* The platform uses 2s complement arithmetic (which should be all platforms these days)
* The integer representation of pointers, at least to the same small object, is "flat". That is if `foo` is a small (say `4*sizeof(void*)`) object and
```cpp

```

### Types

`ref_counted` declares the following public types:

* `ref_counted_base` - a synonym for itself. This allows to refer to the type from its derived types without caring about specific template arguments like `derived::ref_counted_base`
* `weak_value_type` - if weak references are enabled this is the type of weak reference object this class exposes. Otherwise `void`
* `weak_ptr` - if weak references are enabled a synonym for `refcnt_ptr<weak_value_type>`. Otherwise `void`
* `const_weak_ptr` - if weak references are enabled a synonym for `refcnt_ptr<const weak_value_type>`. Otherwise `void`.

### Constants

* `static constexpr bool provides_weak_references` - `true` if this class provides weak references
* `static constexpr bool uses_abstract_base` - `true` if this class uses abstract bases

### Methods

Unless specified otherwise all methods of this class are `noexcept`.

* Copy and move constructors are **deleted**. This is a base class and slicing is meaningless.
* Copy and move assignment operators are similarly **deleted**
* Protected default constructor. Reference count is set to 1 in it.
* Protected destructor. If the class uses abstract base it is virtual and inherited from `abstract_ref_counted`. Otherwise non-virtual.
* Public `void add_ref() const noexcept`. Increments reference count. If the class uses abstract base it is virtual and inherited from `abstract_ref_counted`.
* Public `void sub_ref() const noexcept`. Decrements reference count and destroys the object if it reaches 0. If the class uses abstract base it is virtual and inherited from `abstract_ref_counted`.
* If the class supports weak references then two additional public methods are available
  `weak_ptr get_weak_ptr()` and
  `const_weak_ptr get_weak_ptr() const`. 
  These methods are **not** `noexcept`. Weak reference "control block" is created lazily when a weak reference is requested for the first time. If memory allocation or customized weak reference class constructor throws these methods will throw. Subsequent calls will be `noexcept`. These methods are never virtual.
* Protected `const weak_value_type * get_weak_value() const`. Only meaningfull if the class supports weak references. This is the actual method that retrieves raw pointer to weak reference. Not `nonexcept`. If the class uses abstract base it is virtual and inherited from `abstract_weak_ref_counted`.
* Protected `weak_value_type * make_weak_reference(intptr_t count) const`. Only meaningfull if the class supports weak references. This method is called to create weak reference (control block) when one is needed for the first time. If the class uses abstract base it is virtual and inherited from `abstract_weak_ref_counted`. If the class does not use abstract base it still can be overriden by declaring 
  `some_type * make_weak_reference(intptr_t count) const` in the dervied class (see below). The returned pointer has its own reference count already incremented (as appropriate for a method returning raw pointer).


### Base classes

If `ref_counted` is given `ref_counted_flags::use_abstract_base` flag alone it is directly derived from the following interface

```cpp
class abstract_ref_counted
{
public:
    using refcnt_ptr_traits = ref_counted_traits;
    
    abstract_ref_counted(const abstract_ref_counted &) noexcept = delete;
    abstract_ref_counted & operator=(const abstract_ref_counted &) noexcept = delete;
    abstract_ref_counted(abstract_ref_counted &&) noexcept = delete;
    abstract_ref_counted & operator=(abstract_ref_counted &&) noexcept = delete;
public:
    virtual void add_ref() const noexcept = 0;
    virtual void sub_ref() const noexcept = 0;

protected:
    abstract_ref_counted() noexcept = default;
    virtual ~abstract_ref_counted() noexcept = default;
};
```

If, in addtion, it is given `ref_counted_flags::provide_weak_references` then it derives from an intermediate:

```cpp
class abstract_weak_ref_counted : public abstract_ref_counted
{
public:
    using weak_value_type   = abstract_weak_reference;
    using weak_ptr          = refcnt_ptr<abstract_weak_reference>;
    using const_weak_ptr    = refcnt_ptr<const abstract_weak_reference>;
public:
    weak_ptr get_weak_ptr();
    const_weak_ptr get_weak_ptr() const;
    
private:
    virtual weak_value_type * make_weak_reference(intptr_t count) const = 0;
    virtual const weak_value_type * get_weak_value() const = 0;
};
```

These classes allow handling any of your classes derived from `ref_counted` with these flags polymorphically, as well, as customizing their behavior by overrding virtual function.
The downside of this is the overhead of virtual function calls and lack of inlining when performing reference counting. Your class also gets larger by a virtual pointer.

When `ref_counted_flags::use_abstract_base` is not provided, `ref_counted` has no usable base class and no virtual functions. All access to your derived class is done via CRTP.

### Customizing weak reference type without abstract base

It is possible to customize weak reference type used by your class even if you don't use abstract bases and virtual functions.
`ref_counted` looks via CRTP for a function with the following signature in your derived class:
```cpp
some_type * make_weak_reference(intptr_t count) const
```
where some_type must be a class derived from `isptr::weak_reference` (see below).
If such function exists it will be called instead of the one provided by `ref_counted` itself.

## Class isptr::weak_reference




