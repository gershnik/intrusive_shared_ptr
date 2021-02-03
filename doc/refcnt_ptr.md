# Header `<intrusive_shared_ptr/refcnt_ptr.h>`

<!-- TOC depthfrom:2 -->

- [Class isptr::refcnt_ptr](#class-isptrrefcnt_ptr)
- [Namespace methods](#namespace-methods)

<!-- /TOC -->

## Class isptr::refcnt_ptr

```cpp
template<class T>
using refcnt_ptr = intrusive_shared_ptr<T, typename T::refcnt_ptr_traits>;
```

`refcnt_ptr` is a specialization of [`intrusive_shared_ptr`](intrusive_shared_ptr.md) for a common case where you fully control the pointee's class and can provide traits as a nested type named `refcnt_ptr_traits`. 
A [`ref_counted`](ref_counted.md) base class provides such an implementation so you can easily use `ref_counted` derived classes with `refcnt_ptr`.

## Namespace methods

* `template<class T> constexpr refcnt_ptr<T> refcnt_retain(T * ptr) noexcept`. Creates `refcnt_ptr` from a raw pointer and increments the reference count.
* `template<class T> constexpr refcnt_ptr<T> refcnt_attach(T * ptr) noexcept`. Creates `refcnt_ptr` from a raw pointer without incrementing the reference count.
* `template<class T, class... Args> refcnt_ptr<T> make_refcnt(Args &&... args)`. A convenience function that creates an instance of `T` via `new` and forwards the arguments to its constructor. Equivalent to `refcnt_attach(new T(args))`.
* `template<class T> refcnt_ptr<typename T::weak_value_type> weak_cast(const refcnt_ptr<T> & src)` and <br/>
  `template<class T> refcnt_ptr<const typename T::weak_value_type> weak_cast(const refcnt_ptr<const T> & src)`. <br/>
  If `T` provides a type called `weak_value_type` and a method `get_weak_ptr()` it is assumed to support weak references. These functions provide a convenient "cast" conversion from a strong to a weak pointer wraping the call to `get_weak_ptr()`. 
* `template<class T> refcnt_ptr<typename T::strong_value_type> strong_cast(const refcnt_ptr<T> & src) noexcept` and <br/>
  `template<class T> refcnt_ptr<const typename T::strong_value_type> strong_cast(const refcnt_ptr<const T> & src) noexcept`<br/>
  If `T` provides a type called `strong_value_type` and a method `lock()` it is assumed to be a weak reference. These functions provide a convenient "cast" conversion from a weak to a strong pointer wraping the call to `lock()`. 