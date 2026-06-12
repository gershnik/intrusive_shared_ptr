Header ``com_ptr.h``
====================

Smart pointer support for Microsoft COM interfaces. Available on Windows only.

.. cpp:namespace:: isptr

.. cpp:type:: template<class T> com_shared_ptr = intrusive_shared_ptr<T, com_traits>

   An :cpp:class:`intrusive_shared_ptr` for COM interfaces, counted with
   ``AddRef`` and ``Release``. ``T`` must derive from ``IUnknown``.

   Interfaces returned through an output parameter (the usual COM pattern) work
   with ``get_output_param`` or ``std::out_ptr``.

.. cpp:struct:: com_traits

   Calls ``AddRef`` to add a reference and ``Release`` to drop one, for any
   ``IUnknown``-derived type.

Factory functions
~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<class T> com_shared_ptr<T> com_retain(T * ptr)

   Make a ``com_shared_ptr`` from a raw interface pointer and increment the
   reference count.

.. cpp:function:: template<class T> com_shared_ptr<T> com_attach(T * ptr)

   Make a ``com_shared_ptr`` from a raw interface pointer without changing the
   count.
