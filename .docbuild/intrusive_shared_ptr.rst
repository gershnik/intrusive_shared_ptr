Header ``intrusive_shared_ptr.h``
========================================================

The core smart pointer template and its ``std::atomic`` specialization.

.. cpp:namespace:: isptr

.. cpp:class:: template<class T, class Traits> intrusive_shared_ptr

   A smart pointer that stores ``T *`` and delegates reference-count
   manipulation to ``Traits``.

   .. note::

      The template is declared with ``[[clang::trivial_abi]]`` attribute
      when compiled under Clang. See `trivial_abi <https://github.com/gershnik/intrusive_shared_ptr/blob/master/doc/trivial_abi.md>`_ 
      for the rationale.

   **Traits requirements.** ``Traits`` must expose two static methods with the
   following signatures:

   .. code-block:: cpp

      <unspecified> add_ref(T *) noexcept
      <unspecified> sub_ref(T *) noexcept

   The return value of either method is ignored. The argument is never
   ``nullptr``.

.. cpp:namespace-push:: template<class T, class Traits> intrusive_shared_ptr

Member types
~~~~~~~~~~~~~

.. cpp:type:: pointer = T *
.. cpp:type:: element_type = T
.. cpp:type:: traits_type = Traits

Construction, assignment, destruction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. cpp:function:: intrusive_shared_ptr() noexcept
                  intrusive_shared_ptr(std::nullptr_t) noexcept

   Construct a null pointer.

.. cpp:function:: intrusive_shared_ptr(const intrusive_shared_ptr & src) noexcept
                  intrusive_shared_ptr(intrusive_shared_ptr && src) noexcept

   Copy and move construction from the same type and traits. The move
   constructor performs no changes to the reference count.

.. cpp:function:: template<class Y, class YTraits> intrusive_shared_ptr(const intrusive_shared_ptr<Y, YTraits> & src) noexcept
                  template<class Y, class YTraits> intrusive_shared_ptr(intrusive_shared_ptr<Y, YTraits> && src) noexcept

   Converting copy and move construction from any object of a type ``Y`` such
   that ``Y *`` is convertible to ``T *``, using the same or different traits.
   For the move, if the source and destination traits are the same, no changes
   to the source reference count are performed.

.. cpp:function:: intrusive_shared_ptr & operator=(const intrusive_shared_ptr & src) noexcept
                  intrusive_shared_ptr & operator=(intrusive_shared_ptr && src) noexcept
                  template<class Y, class YTraits> intrusive_shared_ptr & operator=(const intrusive_shared_ptr<Y, YTraits> & src) noexcept
                  template<class Y, class YTraits> intrusive_shared_ptr & operator=(intrusive_shared_ptr<Y, YTraits> && src) noexcept

   Copy and move assignment, with the same type/traits and converting overloads
   as the constructors above. As with the move constructor, a move between
   identical traits performs no changes to the source reference count.

.. cpp:function:: ~intrusive_shared_ptr() noexcept

   Non-virtual destructor.

.. cpp:function:: static intrusive_shared_ptr noref(T * p) noexcept

   Create a smart pointer from a raw pointer **without** modifying the reference
   count (attach).

.. cpp:function:: static intrusive_shared_ptr ref(T * p) noexcept

   Create a smart pointer from a raw pointer and **increment** the reference
   count (retain).

Observers
~~~~~~~~~

.. cpp:function:: T * get() const noexcept

   Return the stored pointer. The reference count is not modified.

.. cpp:function:: T * operator->() const noexcept

   Return the stored pointer. The reference count is not modified.

.. cpp:function:: T & operator*() const noexcept

   Return a reference to the pointee. Undefined if the stored pointer is
   ``nullptr``.

.. cpp:function:: template<class M> M & operator->*(M T::*memptr) const noexcept

   Return the result of ``get()->*memptr``, allowing access through a pointer to
   member of the pointee.

.. cpp:function:: explicit operator bool() const noexcept

   ``true`` if the pointer is non-null.

Modifiers
~~~~~~~~~

.. cpp:function:: T * release() noexcept

   Release ownership of the stored pointer. The object is set to null and the
   caller assumes ownership; the reference count is not adjusted.

.. cpp:function:: void reset() noexcept

   Clear the stored pointer, decrementing the reference count.

.. cpp:function:: void swap(intrusive_shared_ptr & other) noexcept

   Swap with another pointer of the same type. See also the non-member
   :cpp:func:`swap`.

.. cpp:function:: output_param get_output_param() noexcept
                  inout_param get_inout_param() noexcept

   Return a temporary exposing ``operator T**() && noexcept`` that yields a
   ``T**`` aliasing the smart pointer's internal storage, for passing to C
   functions that write back a reference-counted pointer.

   * ``get_output_param`` first resets the pointer to null — use it for **output**
     parameters, where the callee returns a freshly counted pointer.
   * ``get_inout_param`` passes the current value through unchanged — use it for
     **in/out** parameters, where the callee both reads and replaces it.

   .. tip::

      In C++23 and later, prefer ``std::out_ptr`` / ``std::inout_ptr`` (enabled
      by the specializations below) over these methods.

.. cpp:namespace-pop::

Non-member functions
~~~~~~~~~~~~~~~~~~~~~~

All of these are ``noexcept``.

.. cpp:function:: void swap(intrusive_shared_ptr<T, Traits> & lhs, intrusive_shared_ptr<T, Traits> & rhs) noexcept

   Swap two pointers of the same type. Found by ADL only.

.. cpp:function:: bool operator==(...) noexcept
                  bool operator!=(...) noexcept

   Equality and inequality between ``intrusive_shared_ptr`` of compatible types
   (with any traits), raw pointers, and ``nullptr``.

.. cpp:function:: bool operator<(...) noexcept
                  bool operator<=(...) noexcept
                  bool operator>=(...) noexcept
                  bool operator>(...) noexcept

   Ordering between ``intrusive_shared_ptr`` of compatible types (with any
   traits) and raw pointers. On C++20 these are replaced by a single
   ``operator<=>``.

.. cpp:function:: size_t hash_value(const intrusive_shared_ptr<T, Traits> & ptr) noexcept

   Hash of the stored pointer value (e.g. for Boost.Hash).

.. cpp:function:: template<class Char> std::basic_ostream<Char> & operator<<(std::basic_ostream<Char> & str, const intrusive_shared_ptr<T, Traits> & ptr)

   Write the stored pointer value to a stream. Found by ADL only.

.. cpp:function:: template<class Dest> Dest intrusive_const_cast(intrusive_shared_ptr src) noexcept
                  template<class Dest> Dest intrusive_static_cast(intrusive_shared_ptr src) noexcept
                  template<class Dest> Dest intrusive_dynamic_cast(intrusive_shared_ptr src) noexcept

   Perform the equivalent of ``const_cast`` / ``static_cast`` / ``dynamic_cast``
   on the argument. ``Dest`` must be a valid ``intrusive_shared_ptr`` type whose
   underlying type is convertible from the source's via the corresponding cast.
   ``intrusive_dynamic_cast`` returns a null result if the underlying
   ``dynamic_cast`` fails.

Specializations
~~~~~~~~~~~~~~~~

.. cpp:struct:: template<class T, class Traits> std::out_ptr_t<intrusive_shared_ptr<T, Traits>, T *>
.. cpp:struct:: template<class T, class Traits> std::inout_ptr_t<intrusive_shared_ptr<T, Traits>, T *>

   Enable ``std::out_ptr`` / ``std::inout_ptr`` with ``intrusive_shared_ptr``.
   Provided when the standard library offers ``<out_ptr>`` support.

.. cpp:struct:: template<class T, class Traits, class CharT> std::formatter<intrusive_shared_ptr<T, Traits>, CharT>

   Enable ``std::format`` with ``intrusive_shared_ptr``. Provided when the
   standard library offers ``std::format`` support.

.. cpp:struct:: template<class T, class Traits> std::hash<intrusive_shared_ptr<T, Traits>>

   Enable use as a key in unordered associative containers.

.. cpp:class:: template<class Traits, class T> std::atomic<intrusive_shared_ptr<T, Traits>>

   Provides the standard ``std::atomic`` interface for ``intrusive_shared_ptr``.
   It is **not** lock-free.

   .. cpp:type:: value_type = isptr::intrusive_shared_ptr<T, Traits>
   .. cpp:member:: static constexpr bool is_always_lock_free

   .. rubric:: Construction / assignment

   .. cpp:function:: constexpr atomic() noexcept

      Initialize with a null pointer.

   .. cpp:function:: atomic(value_type desired) noexcept
   .. cpp:function:: atomic(const atomic &) = delete
   .. cpp:function:: atomic & operator=(const atomic &) = delete
   .. cpp:function:: ~atomic() noexcept
   .. cpp:function:: value_type operator=(value_type desired) noexcept
   .. cpp:function:: operator value_type() const noexcept

   .. rubric:: Atomic operations

   .. cpp:function:: value_type load(std::memory_order order = std::memory_order_seq_cst) const noexcept
   .. cpp:function:: void store(value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept
   .. cpp:function:: value_type exchange(value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept
   .. cpp:function:: bool compare_exchange_strong(value_type & expected, value_type desired, std::memory_order success, std::memory_order failure) noexcept
                     bool compare_exchange_strong(value_type & expected, value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept
   .. cpp:function:: bool compare_exchange_weak(value_type & expected, value_type desired, std::memory_order success, std::memory_order failure) noexcept
                     bool compare_exchange_weak(value_type & expected, value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept
   .. cpp:function:: bool is_lock_free() const noexcept
