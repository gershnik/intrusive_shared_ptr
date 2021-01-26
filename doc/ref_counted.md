# Header `<intrusive_shared_ptr/ref_counted.h>`

## Class isptr::ref_counted

```cpp
template<class Derived, 
         ref_counted_flags Flags = ref_counted_flags::none, 
         class CountType = default_count_type<Flags>>
class ref_counted;
```

`ref_counted` is meant to be used as base class for any class you want to make reference counted.
It is meant to be used in conjuction with `refcnt_ptr` specialization of `intrusive_shared_ptr` but,
of course, can be used via manual reference counting calls or other smart pointers if desired.

It uses CRTP to access the derived class, avoiding the need for and overhead of virtual functions.
Thus, the first template parameter: `Derived` must be the name of the derived class. Other template
parameters have reasonable defaults and allow customization of reference counting functionality. 
These are described below.


### Customization

`ref_counted` can be customized in the following ways:

* Support weak references. Default: no. Weak reference support adds some performance overhead even for object which never have weak reference taken so it is not enabled by default. In addition weak reference support requires the reference count type to be `intptr_t` which might be wasteful for many applications where `int` is sufficient.
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


