Header ``ref_counted.h``
===============================================

Reusable CRTP base classes that turn an ordinary class into a reference-counted
one, with optional weak-reference and single-threaded support.

.. cpp:namespace:: isptr

Flags
-----

.. cpp:enum-class:: ref_counted_flags : unsigned

   Options controlling a :cpp:class:`ref_counted` instantiation. Values can be
   combined with bitwise ``OR``.

   .. cpp:enumerator:: none = 0

      No options (the default).

   .. cpp:enumerator:: provide_weak_references = 1

      Enable weak references.

   .. cpp:enumerator:: single_threaded = 2

      Enable single-threaded mode.

Class ``isptr::ref_counted``
----------------------------

.. cpp:class:: template<class Derived, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>> ref_counted

   Base class for any class you want to make reference-counted. It is normally
   used together with the :doc:`refcnt_ptr` specialization of
   :cpp:class:`intrusive_shared_ptr`, but can also be driven by manual
   reference-counting calls or other smart pointers.

   It uses CRTP to reach the derived class, avoiding virtual-function overhead,
   so the first template parameter ``Derived`` must be the name of the derived
   class. The remaining parameters customize behaviour and are described below.

Customization
~~~~~~~~~~~~~~

* **Weak references** (default: off). Weak-reference support adds a small
  overhead even for objects that never take a weak reference, so it is opt-in.
  It also forces the count type to ``intptr_t``.
* **Single-threaded mode** (default: off). Reference-count updates are not
  thread-safe, so such objects cannot be shared across threads, but counting is
  faster.
* **Count type** (default: ``int``). Customizable only when weak references are
  *not* enabled; must be a signed integral type. Use a smaller type to shrink
  derived objects when the maximum count is known to be small. With weak
  references the type is always ``intptr_t``.
* Many methods are reached through CRTP calls to ``Derived`` and can therefore
  be "overridden" by declaring a method of the same name in the derived class.
  In particular, :cpp:func:`destroy` is called when the count reaches 0; the
  default calls ``delete`` on the ``Derived`` pointer, and overriding it lets you
  manage differently-allocated objects.

``Flags`` is a combination of :cpp:enum:`ref_counted_flags`. ``CountType`` (when
``provide_weak_references`` is not set) selects the count type; its default,
``default_count_type<Flags>``, is ``int`` without weak references and
``intptr_t`` with them.

Convenience aliases
~~~~~~~~~~~~~~~~~~~~~

.. cpp:type:: template<class Derived> weak_ref_counted = ref_counted<Derived, ref_counted_flags::provide_weak_references>

   ``ref_counted`` with weak references enabled.

.. cpp:type:: template<class Derived, class CountType = default_count_type<ref_counted_flags::single_threaded>> ref_counted_st = ref_counted<Derived, ref_counted_flags::single_threaded, CountType>

   ``ref_counted`` in single-threaded mode (``CountType`` defaults to ``int``).

.. cpp:type:: template<class Derived> weak_ref_counted_st = ref_counted<Derived, ref_counted_flags::provide_weak_references | ref_counted_flags::single_threaded>

   ``ref_counted`` with both weak references and single-threaded mode.

:cpp:class:`ref_counted_adapter` and :cpp:class:`ref_counted_wrapper` expose the
analogous families: ``weak_ref_counted_adapter`` / ``ref_counted_adapter_st`` /
``weak_ref_counted_adapter_st`` and ``weak_ref_counted_wrapper`` /
``ref_counted_wrapper_st`` / ``weak_ref_counted_wrapper_st``.

Usage
~~~~~

Derive from ``ref_counted`` and make it a friend. Keep your destructor private or
protected. Reference-counted objects must be destroyed through the count, not by
a direct ``delete``.

.. code-block:: cpp

   class foo : public ref_counted<foo>
   {
   friend ref_counted;
   private:
       ~foo() noexcept = default;
   };

Rules for constructors and destructors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. warning::

   * **Never increment the reference count in the destructor** (e.g. by making a
     ``refcnt_ptr`` from ``this``). It trips an assert in debug builds and is
     undefined behaviour in release. Creating *weak* references to ``this`` in
     the destructor is fine.
   * **Never hand out** ``this`` **(or a weak reference to it) from a constructor
     if later code in that constructor can throw.** On exception the object fails
     to construct, leaving outside code with an invalid pointer. This is a
     general C++ rule that reference counting does not lift. Handing out ``this``
     is fine if no subsequent constructor code can throw.
   * **Your destructor must not throw.** The counting machinery does not expect
     it and will break.
   * If your class is itself a base, its destructor must be ``virtual``.
     ``ref_counted`` invokes the destructor of the ``Derived`` argument. (Not
     applicable if ``Derived`` is the most-derived class.)

Limitations
~~~~~~~~~~~

Without weak references the implementation is standard, portable, and limited
only by requiring the live reference count to stay within
``[0, std::numeric_limits<CountType>::max()]``.

With weak references it multiplexes a pointer and the count in one word, which
assumes: two's-complement arithmetic; ``alignof(intptr_t) > 1``; and that a
pointer to an over-aligned object has its lowest bit clear when cast to
``uintptr_t``. These hold on all current platforms.

.. cpp:namespace-push:: template<class Derived, ref_counted_flags Flags, class CountType> ref_counted

Member types
~~~~~~~~~~~~~

.. cpp:type:: ref_counted_base = ref_counted

   A synonym for the class itself, letting derived types refer to it as
   ``Derived::ref_counted_base`` without spelling out the template arguments.

.. cpp:type:: weak_value_type

   The weak-reference object type this class exposes if weak references are
   enabled; otherwise ``void``.

.. cpp:type:: weak_ptr

   ``refcnt_ptr<weak_value_type>`` if weak references are enabled; otherwise
   ``void``.

.. cpp:type:: const_weak_ptr

   ``refcnt_ptr<const weak_value_type>`` if weak references are enabled;
   otherwise ``void``.

Constants
~~~~~~~~~

.. cpp:member:: static constexpr bool provides_weak_references

   ``true`` if this class provides weak references.

.. cpp:member:: static constexpr bool single_threaded

   ``true`` if this class is single-threaded.

Methods
~~~~~~~

Unless noted otherwise, all methods are ``noexcept``. The copy and move
constructors and assignment operators are **deleted** (this is a base class, so
slicing would be meaningless). The default constructor is **protected** and sets
the reference count to 1; the destructor is **protected** as well.

.. cpp:function:: void add_ref() const noexcept

   Increment the reference count. Overridable.

.. cpp:function:: void sub_ref() const noexcept

   Decrement the reference count and destroy the object when it reaches 0.
   Overridable.

.. cpp:function:: void destroy() const noexcept

   *Protected.* Called when the count reaches 0; the default calls ``delete`` on
   the ``Derived`` pointer. Overridable.

.. cpp:function:: weak_ptr get_weak_ptr()
                  const_weak_ptr get_weak_ptr() const

   Obtain a weak pointer to this object (weak-reference builds only). **Not**
   ``noexcept``: the control block is created lazily on first use, so allocation
   or a custom weak-reference constructor may throw. Later calls do not throw.

.. cpp:function:: const weak_value_type * get_weak_value() const

   *Protected, not* ``noexcept``. The actual retrieval of the raw weak-reference
   pointer behind :cpp:func:`get_weak_ptr`. Overridable.

.. cpp:function:: weak_value_type * make_weak_reference(intptr_t count) const

   *Protected, not* ``noexcept`` (it allocates). Creates the control block on
   first use; the returned pointer already has its own count incremented.
   Overridable. See :ref:`customizing-weak-reference-type`.

.. cpp:namespace-pop::

.. _customizing-weak-reference-type:

Customizing the weak reference type
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can change the weak-reference type by "overriding"
:cpp:func:`~ref_counted::make_weak_reference`. ``ref_counted`` looks via CRTP for
a member of this form in your derived class:

.. code-block:: cpp

   some_type * make_weak_reference(intptr_t count) const

where ``some_type`` derives from :cpp:class:`weak_reference`. If present, it is
called instead of the built-in one and its return type becomes the weak-reference
type.

Class ``isptr::weak_reference``
-------------------------------

.. cpp:class:: template<class Owner> weak_reference

   The control block representing a weak reference to a :cpp:class:`ref_counted`
   class. Usually used as-is, but can serve as a base for a customized control
   block. ``Owner`` is the class it refers to.

   Internally it is reference-counted itself and also manages the count of the
   referenced object; instances are created on demand when the first weak
   reference is requested.

Customization
~~~~~~~~~~~~~~

Derive your own class from ``weak_reference`` and pair it with your
``ref_counted`` class (see :ref:`customizing-weak-reference-type`). As with
``ref_counted``, several methods are reached via CRTP and can be overridden; in
particular :cpp:func:`~weak_reference::destroy` defaults to ``delete`` on your
class pointer.

Usage
~~~~~

You rarely name ``weak_reference`` directly. It is available as the
``weak_value_type`` of your ``ref_counted`` class. With ``refcnt_ptr`` you
convert between strong and weak pointers like so:

.. code-block:: cpp

   auto original = refcnt_attach(new your_object());

   auto weak = weak_cast(original);
   // or: your_object::weak_ptr weak = weak_cast(original);

   auto strong = strong_cast(weak);
   assert(strong == original || strong == nullptr);

The member functions work too:

.. code-block:: cpp

   auto weak   = original->get_weak_ptr();
   auto strong = weak->lock();

``const``-ness propagates: a strong pointer to const yields a weak pointer to
const and vice versa.

.. cpp:namespace-push:: template<class Owner> weak_reference

Member types
~~~~~~~~~~~~~

.. cpp:type:: strong_value_type = Owner
.. cpp:type:: strong_ptr = refcnt_ptr<strong_value_type>
.. cpp:type:: const_strong_ptr = refcnt_ptr<const strong_value_type>

Methods
~~~~~~~

Unless noted otherwise, all methods are ``noexcept``. The copy and move
constructors and assignment operators are **deleted**. The destructor is
**protected**.

.. cpp:function:: constexpr weak_reference(intptr_t initial_strong, Owner * owner) noexcept

   *Protected.* ``initial_strong`` is the owner's reference count at the moment
   the control block is created (it can be any value reached beforehand);
   ``owner`` is a raw pointer to the ``Owner``.

.. cpp:function:: void destroy() const

   *Protected. Unlike the rest of this class it is not* ``noexcept``. Called when
   this control block's own count reaches 0; the default calls ``delete`` on the
   derived pointer. Overridable.

.. cpp:function:: void add_ref() const noexcept

   Increment this control block's own reference count. Overridable.

.. cpp:function:: void sub_ref() const noexcept

   Decrement this control block's own count and destroy it at 0. The ``Owner``
   always holds a reference to its control block, so the block outlives the
   owner. Overridable.

.. cpp:function:: const_strong_ptr lock() const noexcept
                  strong_ptr lock() noexcept

   Obtain a strong reference to the ``Owner``, or a null pointer if it no longer
   exists.

.. cpp:function:: void add_owner_ref() noexcept
                  void sub_owner_ref() noexcept

   *Protected.* Manage the ``Owner``'s reference count. Overridable.

.. cpp:function:: strong_value_type * lock_owner() const noexcept

   *Protected.* Performs the actual lock, returning the owner with its count
   incremented, or null. Overridable.

.. cpp:function:: void on_owner_destruction() const noexcept

   *Protected.* A no-op customization point invoked *after* the ``Owner`` is
   destroyed (when its strong count hits 0). You cannot access or resurrect the
   owner from it.

.. cpp:namespace-pop::

Class ``isptr::ref_counted_adapter``
------------------------------------

.. cpp:class:: template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>> ref_counted_adapter

   Publicly derives from a non-reference-counted class ``T`` **and** from
   :cpp:class:`ref_counted`. The trailing template parameters are forwarded to
   ``ref_counted``.

   Aliases ``weak_ref_counted_adapter``, ``ref_counted_adapter_st`` and
   ``weak_ref_counted_adapter_st`` mirror the ``ref_counted`` family.

   .. cpp:function:: template<class... Args> ref_counted_adapter(Args &&... args)

      Perfectly forwards to ``T``'s constructor; ``noexcept`` if that constructor
      is.

   .. cpp:function:: ~ref_counted_adapter() noexcept

      *Protected.*

Class ``isptr::ref_counted_wrapper``
------------------------------------

.. cpp:class:: template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>> ref_counted_wrapper

   Stores a member of class ``T`` and derives from :cpp:class:`ref_counted`. The
   trailing template parameters are forwarded to ``ref_counted``.

   Aliases ``weak_ref_counted_wrapper``, ``ref_counted_wrapper_st`` and
   ``weak_ref_counted_wrapper_st`` mirror the ``ref_counted`` family.

   .. cpp:member:: T wrapped

      The wrapped instance of ``T``.

   .. cpp:function:: template<class... Args> ref_counted_wrapper(Args &&... args)

      Perfectly forwards to ``wrapped``'s constructor; ``noexcept`` if ``T``'s
      constructor is.

   .. cpp:function:: ~ref_counted_wrapper() noexcept

      *Protected.*
