Header ``refcnt_ptr.h``
==============================================

A convenience layer over :cpp:class:`~isptr::intrusive_shared_ptr` for classes
that supply their own traits, plus the free functions that go with it.

.. cpp:namespace:: isptr

.. cpp:type:: template<class T> refcnt_ptr = intrusive_shared_ptr<T, typename T::refcnt_ptr_traits>

   A specialization of :cpp:class:`intrusive_shared_ptr` for the common case
   where you fully control the pointee's class and can provide traits as a
   nested type named ``refcnt_ptr_traits``. The :cpp:class:`ref_counted` base class
   provides such an implementation, so ``ref_counted``-derived classes work with
   ``refcnt_ptr`` out of the box.

Factory functions
~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<class T> refcnt_ptr<T> refcnt_retain(T * ptr) noexcept

   Create a ``refcnt_ptr`` from a raw pointer and **increment** the reference
   count.

.. cpp:function:: template<class T> refcnt_ptr<T> refcnt_attach(T * ptr) noexcept

   Create a ``refcnt_ptr`` from a raw pointer **without** incrementing the
   reference count.

.. cpp:function:: template<class T, class... Args> refcnt_ptr<T> make_refcnt(Args &&... args)

   Create an instance of ``T`` via ``new``, forwarding the arguments to its
   constructor. Equivalent to ``refcnt_attach(new T(args...))``.

Weak/strong conversions
~~~~~~~~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<class T> refcnt_ptr<typename T::weak_value_type> weak_cast(const refcnt_ptr<T> & src)
                  template<class T> refcnt_ptr<const typename T::weak_value_type> weak_cast(const refcnt_ptr<const T> & src)

   If ``T`` provides a type ``weak_value_type`` and a method ``get_weak_ptr()``,
   it is assumed to support weak references. These functions provide a convenient
   "cast" from a strong to a weak pointer, wrapping the call to
   ``get_weak_ptr()``.

.. cpp:function:: template<class T> refcnt_ptr<typename T::strong_value_type> strong_cast(const refcnt_ptr<T> & src) noexcept
                  template<class T> refcnt_ptr<const typename T::strong_value_type> strong_cast(const refcnt_ptr<const T> & src) noexcept

   If ``T`` provides a type ``strong_value_type`` and a method ``lock()``, it is
   assumed to be a weak reference. These functions provide a convenient "cast"
   from a weak to a strong pointer, wrapping the call to ``lock()``.
