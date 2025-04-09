# Header `<intrusive_shared_ptr/intrusive_shared_ptr.h>`

## Class isptr::intrusive_shared_ptr

```cpp
template<class T, class Traits>
class ISPTR_TRIVIAL_ABI intrusive_shared_ptr;
```

Macro `ISPTR_TRIVIAL_ABI` expands to `[[clang::trivial_abi]]` when compiled under clang.

### Traits requirements

Traits must expose two static methods with the following signatures:

```cpp
<unspecified> add_ref(T *) noexcept
<unspecified> sub_ref(T *) noexcept
```

Return value of either method is ignored. The argument is never `nullptr`.

### Namespace Types

- `template<class Traits, class T> constexpr bool are_intrusive_shared_traits` checks whether given Traits satisfy the traits requirements above for the given type T.
- `template<class T> using is_intrusive_shared_ptr = ...` type trait that is `std::true_type` or `std::false_type` depending on whether T is a valid `intrusive_shared_ptr` type.
- `template<class T> bool constexpr is_intrusive_shared_ptr_v = is_intrusive_shared_ptr<T>::value;`


### Nested Types

```cpp
using pointer = T *;
using element_type = T;
using traits_type = Traits;
```

### Operations

#### Construction/assignment/destruction

- Default constructor. `noexcept`, produces a null pointer
- Constructor from `nullptr`. Same as above
- Copy constructor. `noexcept`. Accepts:
  - Objects of the same type and traits
  - Objects that point to `Y` such as `Y *` is convertible to `T *` using the same or different traits. 
- Move constructor. `noexcept`. Accepts:
  - Objects of the same type and traits. No changes to reference count are performed.
  - Objects that point to `Y` such as `Y *` is convertible to `T *` using the same or different traits.
  If the source and destination traits are the same no changes to source reference count are performed. 
- Copy assignment. `noexcept`. Accepts:
  - Objects of the same type and traits
  - Objects that point to `Y` such as `Y *` is convertible to `T *` using the same or different traits. 
- Move assignment. `noexcept`. Accepts:
  - Objects of the same type and traits 
  - Objects that point to `Y` such as `Y *` is convertible to `T *` using the same or different traits. 
  If the source and destination traits are the same no changes to source reference count are performed.
- Destructor. `noexcept`. Non-virtual.

- `static intrusive_shared_ptr<T, Traits> noref() noexcept` Creates a smart pointer from a raw pointer without modifying the reference count. 
- `static intrusive_shared_ptr<T, Traits> ref() noexcept` Creates a smart pointer from a raw pointer and increments reference count.

#### Instance methods

- `T * get() const noexcept` Returns the stored pointer. Reference count is not modified.
- `T * operator->() const noexcept` Returns the stored pointer. Reference count is not modified.
- `T & operator*() const noexcept` Returns the reference to pointee. Undefined if stored pointer is nullptr
- `template<class M> M & operator->*(M T::*memptr) const noexcept` Returns the result of `'stored pointer'->*memptr`
   This allows access via pointee's pointer to members
- `explicit operator bool() const noexcept` true if the pointer is non null
- Before C++23: `output_param get_output_param() noexcept` returns a temporary object that exposes `operator T**() && noexcept`. This can
   be passed as an output parameter to C functions that return a reference counted pointer. The destructor of the temporary
   will fill this pointer with the returned result.
   In C++23 and above use `std::out_ptr` or `std::inout_ptr`instead.
- `T * release() noexcept` Releases the stored pointer. The object is set to null and calling code assumes ownership of the
   pointer. No adjustment is made to the reference count.
- `void reset() noexcept` Clears the stored pointer. The reference count is decremented.
- `void swap(intrusive_shared_ptr<T, Traits> & other) noexcept` and <br/>
  `friend void swap(intrusive_shared_ptr<T, Traits> & lhs, intrusive_shared_ptr<T, Traits> & rhs) noexcept` (ADL only)
   Perform swap of pointers of the same type

#### Namespace methods

- Equality: `==`, `!=`. All `noexcept`. Work between `intrusive_shared_ptr` of compatible types and
  any traits as well as raw pointers and nullptr.
- Comparisons: `<`, `<=`, `>=`, `>`. All `noexcept`. On C++20 these are replaced by `<=>`. Work between `intrusive_shared_ptr` of compatible types and
  any traits as well as raw pointers.
- Namespace method: `template<class Char> friend std::basic_ostream<Char> & operator<<(std::basic_ostream<Char> & str, const intrusive_shared_ptr<T, Traits> & ptr)` (ADL only) outputs the stored pointer value to a stream

- `Dest intrusive_const_cast<Dest>(intrusive_shared_ptr src) noexcept`. Performs an equivalent of `const_cast` on passed argument. The destination type needs to be a valid `intrusive_shared_ptr` type whose underlying type is convertible from source's via `const_cast`.
- `Dest intrusive_static_cast<Dest>(intrusive_shared_ptr src) noexcept`. Performs an equivalent of `static_cast` on passed argument. The destination type needs to be a valid `intrusive_shared_ptr` type whose underlying type is convertible from source's via `static_cast`.
- `Dest intrusive_dynamic_cast<Dest>(intrusive_shared_ptr src) noexcept`. Performs an equivalent of `dynamic_cast` on passed argument. The destination type needs to be a valid `intrusive_shared_ptr` type whose underlying type is convertible from source's via `dynamic_cast`. Returns a null result if underlying `dynamic_cast` fails.



## Class std::atomic&lt;isptr::intrusive_shared_ptr&gt;

Provides the standard set of `std::atomic` functionality:

- `using value_type = isptr::intrusive_shared_ptr<T, Traits>`
- `static constexpr bool is_always_lock_free`
- `constexpr atomic() noexcept`. Initializes with null pointer.
- `atomic(value_type desired) noexcept`
- `atomic(const atomic&) = delete` Deleted!
- `void operator=(const atomic&) = delete` Deleted!
- `~atomic() noexcept`
- `void operator=(value_type desired) noexcept`
- `operator value_type() const noexcept`
- `value_type load(memory_order order = memory_order_seq_cst) const noexcept`
- `void store(value_type desired, memory_order order = memory_order_seq_cst) noexcept`
- `value_type exchange(value_type desired, memory_order order = memory_order_seq_cst) noexcept`
- `bool compare_exchange_strong(value_type & expected, value_type desired, memory_order success, memory_order failure) noexcept`
- `bool compare_exchange_strong(value_type & expected, value_type desired, memory_order order = memory_order_seq_cst) noexcept`
- `bool compare_exchange_weak(value_type & expected, value_type desired, memory_order success, memory_order failure) noexcept`
- `bool compare_exchange_weak(value_type & expected, value_type desired, memory_order order = memory_order_seq_cst) noexcept`
- `bool is_lock_free() const noexcept`










   
