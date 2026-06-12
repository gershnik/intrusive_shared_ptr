Header ``apple_cf_ptr.h``
=========================

Smart pointer support for Apple Core Foundation objects. Available on macOS only.

.. cpp:namespace:: isptr

.. cpp:type:: template<class T> cf_ptr = intrusive_shared_ptr<std::remove_pointer_t<T>, cf_traits>

   An :cpp:class:`intrusive_shared_ptr` for Core Foundation types, counted with
   ``CFRetain`` and ``CFRelease``. ``T`` is a CF pointer type such as
   ``CFStringRef``, so you spell it ``cf_ptr<CFStringRef>``.

.. cpp:struct:: cf_traits

   Calls ``CFRetain`` to add a reference and ``CFRelease`` to drop one. Works for
   any ``CFTypeRef``.

Factory functions
~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<class T> cf_ptr<T *> cf_retain(T * ptr)

   Make a ``cf_ptr`` from a raw CF pointer and increment the reference count.

.. cpp:function:: template<class T> cf_ptr<T *> cf_attach(T * ptr)

   Make a ``cf_ptr`` from a raw CF pointer without changing the count.
