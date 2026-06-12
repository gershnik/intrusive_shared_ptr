Header ``python_ptr.h``
=======================

Smart pointer support for CPython objects. Needs the Python C API headers.

.. cpp:namespace:: isptr

.. cpp:type:: template<class T> py_ptr = intrusive_shared_ptr<T, py_traits>

   An :cpp:class:`intrusive_shared_ptr` for Python objects, counted with
   ``Py_INCREF`` and ``Py_DECREF``. ``T`` is normally ``PyObject`` or
   ``PyTypeObject``.

.. cpp:struct:: py_traits

   Calls ``Py_INCREF`` to add a reference and ``Py_DECREF`` to drop one, for
   ``PyObject`` and ``PyTypeObject``.

Factory functions
~~~~~~~~~~~~~~~~~~

.. cpp:function:: template<class T> py_ptr<T> py_retain(T * ptr)

   Make a ``py_ptr`` from a raw object pointer and increment the reference count.

.. cpp:function:: template<class T> py_ptr<T> py_attach(T * ptr)

   Make a ``py_ptr`` from a raw object pointer without changing the count.
