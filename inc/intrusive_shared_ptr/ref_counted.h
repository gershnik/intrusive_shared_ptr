#ifndef HEADER_REF_COUNTED_H_INCLUDED
#define HEADER_REF_COUNTED_H_INCLUDED

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

#include <atomic>
#include <cassert>

namespace isptr
{

    //MARK:- helpers

    namespace internal
    {
        template<bool Val, class... Args>
        static constexpr bool dependent_bool = Val;
    }

    //MARK:- ref_counted_flags

    enum class ref_counted_flags : unsigned
    {
        none = 0,
        provide_weak_references = 1,
        use_abstract_base = 2,
    };

    constexpr ref_counted_flags operator|(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) | unsigned(rhs)); }
    constexpr ref_counted_flags operator&(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) & unsigned(rhs)); }
    constexpr ref_counted_flags operator^(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) ^ unsigned(rhs)); }
    constexpr ref_counted_flags operator~(ref_counted_flags arg) noexcept
        { return ref_counted_flags(~unsigned(arg)); }

    constexpr bool contains(ref_counted_flags val, ref_counted_flags flag) noexcept
        { return (val & flag) == flag;   }

    //MARK:- Forward Declarations

    template<ref_counted_flags Flags>
    using default_count_type = std::conditional_t<
                                contains(Flags, ref_counted_flags::provide_weak_references),
                                intptr_t, int>;

    template<class Derived, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted;

    template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted_adapter;

    template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted_wrapper;

    template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
    using weak_ref_counted = ref_counted<Derived, Flags | ref_counted_flags::provide_weak_references>;

    template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
    using weak_ref_counted_adapter = ref_counted_adapter<Derived, Flags | ref_counted_flags::provide_weak_references>;

    template<class Owner, bool UsesAbstractBase = Owner::uses_abstract_base>
    class weak_reference;


    //MARK:-

    struct ref_counted_traits
    {
        template<class T>
        static void add_ref(const T * obj) noexcept
            { obj->add_ref(); }
        
        template<class T>
        static void sub_ref(const T * obj) noexcept
            { obj->sub_ref(); }
    };

    //MARK:- Bases

    class abstract_weak_reference;

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

    class abstract_weak_ref_counted : public abstract_ref_counted
    {
    public:
        using weak_value_type   = abstract_weak_reference;
        using weak_ptr          = intrusive_shared_ptr<abstract_weak_reference, ref_counted_traits>;
        using const_weak_ptr    = intrusive_shared_ptr<const abstract_weak_reference, ref_counted_traits>;
    public:
        virtual weak_ptr get_weak_ptr()
            { return weak_ptr::noref(const_cast<abstract_weak_reference *>(const_cast<const abstract_weak_ref_counted *>(this)->get_weak_value())); }
        virtual const_weak_ptr get_weak_ptr() const
            { return const_weak_ptr::noref(this->get_weak_value()); }
        
    private:
        virtual weak_value_type * make_weak_reference(intptr_t count) const = 0;
        virtual const weak_value_type * get_weak_value() const = 0;
    };

    class abstract_weak_reference : public abstract_ref_counted
    {
    public:
        using strong_value_type = abstract_weak_ref_counted;
        using strong_ptr = intrusive_shared_ptr<strong_value_type, ref_counted_traits>;
        using const_strong_ptr = intrusive_shared_ptr<const strong_value_type, ref_counted_traits>;
        
        const_strong_ptr lock() const noexcept
            { return const_strong_ptr::noref(this->lock_owner()); }
        
        strong_ptr lock() noexcept
            { return strong_ptr::noref(this->lock_owner()); }
    protected:
        constexpr abstract_weak_reference() noexcept = default;
        virtual ~abstract_weak_reference() noexcept = default;

    private:
        virtual void add_owner_ref() noexcept = 0;
        virtual void sub_owner_ref() noexcept = 0;
        virtual strong_value_type * lock_owner() const noexcept = 0;
        virtual void on_owner_destruction() const noexcept = 0;
    };

    class non_abstract_ref_counted
    {
    public:
        using refcnt_ptr_traits = ref_counted_traits;
        
        non_abstract_ref_counted(const non_abstract_ref_counted &) noexcept = delete;
        non_abstract_ref_counted & operator=(const non_abstract_ref_counted &) noexcept = delete;
        non_abstract_ref_counted(non_abstract_ref_counted &&) noexcept = delete;
        non_abstract_ref_counted & operator=(non_abstract_ref_counted &&) noexcept = delete;
    protected:
        non_abstract_ref_counted() noexcept = default;
        ~non_abstract_ref_counted() noexcept = default;
    };

    //MARK:- weak_reference_traits

    template<class Derived, bool ProvideWeakReferences, bool UsesAbstractBase>
    struct weak_reference_traits;

    template<class Derived, bool UsesAbstractBase>
    struct weak_reference_traits<Derived, true, UsesAbstractBase>
    {
        using weak_value_type = weak_reference<Derived, UsesAbstractBase>;
        using weak_ptr = intrusive_shared_ptr<weak_value_type, ref_counted_traits>;
        using const_weak_ptr = intrusive_shared_ptr<const weak_value_type, ref_counted_traits>;
    };

    template<class Derived, bool UsesAbstractBase>
    struct weak_reference_traits<Derived, false, UsesAbstractBase>
    {
        using weak_value_type = void;
        using weak_ptr = void;
        using const_weak_ptr = void;
    };

    //MARK:-

    template<class Derived, ref_counted_flags Flags, class CountType>
    class ref_counted : public
        std::conditional_t<contains(Flags, ref_counted_flags::use_abstract_base),
            std::conditional_t<contains(Flags, ref_counted_flags::provide_weak_references),
                    abstract_weak_ref_counted,
                    abstract_ref_counted>,
            non_abstract_ref_counted>
    {
    template<class Owner, bool UsesAbstractBase> friend class weak_reference;
    public:
        using ref_counted_base = ref_counted;
        
        static constexpr bool provides_weak_references = contains(Flags, ref_counted_flags::provide_weak_references);
        static constexpr bool uses_abstract_base = contains(Flags, ref_counted_flags::use_abstract_base);
        
    private:
        using wr_traits = weak_reference_traits<Derived, ref_counted::provides_weak_references, ref_counted::uses_abstract_base>;
        
    public:
        using weak_value_type   = typename ref_counted::wr_traits::weak_value_type;
        using weak_ptr          = typename ref_counted::wr_traits::weak_ptr;
        using const_weak_ptr    = typename ref_counted::wr_traits::const_weak_ptr;
        
    private:
        static_assert(!ref_counted::provides_weak_references || (ref_counted::provides_weak_references && std::is_same_v<CountType, intptr_t>),
                      "CountType must be intptr_t (the default) when providing weak references");
        static_assert(std::is_integral_v<CountType>, "CountType must be an integral type");
        static_assert(std::atomic<CountType>::is_always_lock_free,
                      "CountType must be such that std::atomic<CountType> is alwayd lock free");
        
        using count_type = CountType;

    public:
        ref_counted(const ref_counted &) noexcept = delete;
        ref_counted & operator=(const ref_counted &) noexcept = delete;
        ref_counted(ref_counted &&) noexcept = delete;
        ref_counted & operator=(ref_counted &&) noexcept = delete;
        
        void add_ref() const noexcept;
        void sub_ref() const noexcept;
        
        template<class X = Derived, class = std::enable_if_t<internal::dependent_bool<ref_counted::provides_weak_references, X>> >
        weak_ptr get_weak_ptr()
            { return weak_ptr::noref(const_cast<weak_reference<X> *>(const_cast<const ref_counted *>(this)->get_weak_value())); }
        
        template<class X = Derived, class = std::enable_if_t<internal::dependent_bool<ref_counted::provides_weak_references, X>> >
        const_weak_ptr get_weak_ptr() const
            { return const_weak_ptr::noref(this->get_weak_value()); }
        
    protected:
        ref_counted() noexcept = default;
        ~ref_counted() noexcept;
        
        const weak_value_type * get_weak_value() const;
        
        weak_value_type * make_weak_reference(intptr_t count) const
        {
            auto non_const_derived = static_cast<Derived *>(const_cast<ref_counted *>(this));
            return new weak_value_type(count, non_const_derived);
        }

    private:
        void destroy() const noexcept
        {
            if constexpr (!ref_counted::uses_abstract_base)
                delete static_cast<const Derived *>(this);
            else
                delete this;
        }

        auto call_make_weak_reference(intptr_t count) const
        {
            if constexpr (ref_counted::provides_weak_references)
            {
                if constexpr (!ref_counted::uses_abstract_base)
                    return static_cast<const Derived *>(this)->make_weak_reference(count);
                else
                    return this->make_weak_reference(count);
            }
        }
        
        template<class X>
        static X * decode_pointer(intptr_t value) noexcept
            { return (X *)(uintptr_t(value) << 1); }

        template<class X>
        static intptr_t encode_pointer(X * ptr) noexcept
            { return (uintptr_t(ptr) >> 1) | uintptr_t(std::numeric_limits<intptr_t>::min()); }

        static bool is_encoded_pointer(intptr_t value) noexcept
            { return value < 0; }
    private:
        mutable std::atomic<count_type> m_count = 1;
    };

    template<class Owner, bool UsesAbstractBase>
    class weak_reference : public std::conditional_t<UsesAbstractBase, abstract_weak_reference, non_abstract_ref_counted>
    {
    template<class T, ref_counted_flags Flags, class CountType> friend class ref_counted;
    public:
        using strong_value_type = Owner;
        using strong_ptr = intrusive_shared_ptr<strong_value_type, ref_counted_traits>;
        using const_strong_ptr = intrusive_shared_ptr<const strong_value_type, ref_counted_traits>;
        
    public:
        void add_ref() const noexcept;
        void sub_ref() const noexcept;
        
        template<class X = Owner>
        const_strong_ptr lock() const noexcept
            { return const_strong_ptr::noref(this->lock_owner()); }
        
        template<class X = Owner>
        strong_ptr lock() noexcept
            { return strong_ptr::noref(this->lock_owner()); }

    protected:
        constexpr weak_reference(intptr_t initial_strong, Owner * owner) noexcept:
            m_strong(initial_strong),
            m_owner(owner)
        {}

        ~weak_reference() noexcept = default;
        
        void add_owner_ref() noexcept;
        void sub_owner_ref() noexcept;
        
        std::conditional_t<UsesAbstractBase,
            abstract_weak_reference::strong_value_type,
            strong_value_type> *
        lock_owner() const noexcept;
        
        void on_owner_destruction() const noexcept
        {}
        
    private:
        void destroy() const
        {
            if constexpr (!UsesAbstractBase)
            {
                using derived = std::remove_pointer_t<decltype(std::declval<Owner>().call_make_weak_reference(0))>;
                delete static_cast<const derived *>(this);
            }
            else
            {
                delete this;
            }
        }
        
        void call_on_owner_destruction() const noexcept
        {
            if constexpr (!Owner::uses_abstract_base)
            {
                using derived = std::remove_pointer_t<decltype(std::declval<Owner>().call_make_weak_reference(0))>;
                static_cast<const derived *>(this)->on_owner_destruction();
            }
            else
            {
                on_owner_destruction();
            }
        }
        
    private:
        mutable std::atomic<intptr_t> m_count = 2;
        mutable std::atomic<intptr_t> m_strong = 0;
        Owner * m_owner = nullptr;
    };

    template<class T, ref_counted_flags Flags, class CountType>
    class ref_counted_adapter : public ref_counted<ref_counted_adapter<T, Flags, CountType>, Flags, CountType>, public T
    {
    friend ref_counted<ref_counted_adapter<T, Flags, CountType>, Flags, CountType>;
    public:
        template<class... Args, class=std::enable_if_t<std::is_constructible_v<T, Args...>>>
        ref_counted_adapter(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))):
            T(std::forward<Args>(args)...)
        {}

    protected:
        ~ref_counted_adapter() noexcept = default;
    };

    template<class T, ref_counted_flags Flags, class CountType>
    class ref_counted_wrapper : public ref_counted<ref_counted_wrapper<T, Flags, CountType>, Flags, CountType>
    {
    friend ref_counted<ref_counted_wrapper<T, Flags, CountType>, Flags, CountType>;
    public:
        template<class... Args, class=std::enable_if_t<std::is_constructible_v<T, Args...>>>
        ref_counted_wrapper(Args&&... args) noexcept(noexcept(T(std::forward<Args>(args)...))):
            wrapped(std::forward<Args>(args)...)
        {}
        
        T wrapped;

    protected:
        ~ref_counted_wrapper() noexcept = default;
    };


    //MARK:- Implementation

    template<class Owner, bool UsesAbstractBase>
    inline void weak_reference<Owner, UsesAbstractBase>::add_ref() const noexcept
    {
        [[maybe_unused]] auto oldcount = this->m_count.fetch_add(1, std::memory_order_relaxed);
        assert(oldcount > 0);
        assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
    }

    template<class Owner, bool UsesAbstractBase>
    inline void weak_reference<Owner, UsesAbstractBase>::sub_ref() const noexcept
    {
        auto oldcount = this->m_count.fetch_sub(1, std::memory_order_release);
        assert(oldcount > 0);
        if (oldcount == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->destroy();
        }
    }

    template<class Owner, bool UsesAbstractBase>
    inline void weak_reference<Owner, UsesAbstractBase>::add_owner_ref() noexcept
    {
        [[maybe_unused]] auto oldcount = this->m_strong.fetch_add(1, std::memory_order_relaxed);
        assert(oldcount > 0);
        assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
    }

    template<class Owner, bool UsesAbstractBase>
    inline void weak_reference<Owner, UsesAbstractBase>::sub_owner_ref() noexcept
    {
        auto oldcount = this->m_strong.fetch_sub(1, std::memory_order_release);
        assert(oldcount > 0);
        if (oldcount == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            auto owner = this->m_owner;
            this->m_owner = nullptr;
            owner->destroy(); //this can cascade to deleting ourselves so must be the last thing
        }
    }

    template<class Owner, bool UsesAbstractBase>
    inline
    auto weak_reference<Owner, UsesAbstractBase>::lock_owner() const noexcept ->
        std::conditional_t<UsesAbstractBase, abstract_weak_reference::strong_value_type, strong_value_type> *
    {
        for (intptr_t value = this->m_strong.load(std::memory_order_relaxed); ; )
        {
            assert(value >= 0);
            
            if (value == 0)
                return nullptr;

            if (this->m_strong.compare_exchange_strong(value, value + 1, std::memory_order_release, std::memory_order_relaxed))
                return this->m_owner;
        }
    }

    template<class Derived, ref_counted_flags Flags, class CountType>
    inline void ref_counted<Derived, Flags, CountType>::add_ref() const noexcept
    {
        if constexpr(!ref_counted::provides_weak_references)
        {
            [[maybe_unused]] auto oldcount = this->m_count.fetch_add(1, std::memory_order_relaxed);
            assert(oldcount > 0);
            assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
        }
        else
        {
            for(intptr_t value = this->m_count.load(std::memory_order_relaxed); ; )
            {
                assert(value != 0);
                if (!ref_counted::is_encoded_pointer(value))
                {
                    assert(value < std::numeric_limits<decltype(value)>::max());
                    if (this->m_count.compare_exchange_strong(value, value + 1, std::memory_order_release, std::memory_order_relaxed))
                        return;
                }
                else
                {
                    auto ptr = ref_counted::decode_pointer<weak_value_type>(value);
                    ptr->add_owner_ref();
                    return;
                }
            }
        }
    }
        
    template<class Derived, ref_counted_flags Flags, class CountType>
    inline void ref_counted<Derived, Flags, CountType>::sub_ref() const noexcept
    {
        if constexpr(!ref_counted::provides_weak_references)
        {
            auto oldcount = this->m_count.fetch_sub(1, std::memory_order_release);
            assert(oldcount > 0);
            if (oldcount == 1)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                this->destroy();
            }
        }
        else
        {
            for (intptr_t value = this->m_count.load(std::memory_order_relaxed); ; )
            {
                assert(value != 0);
                if (!ref_counted::is_encoded_pointer(value))
                {
                    if (this->m_count.compare_exchange_strong(value, value - 1, std::memory_order_release, std::memory_order_relaxed))
                    {
                        if (value == 1)
                        {
                            std::atomic_thread_fence(std::memory_order_acquire);
                            this->destroy();
                        }
                        return;
                    }
                }
                else
                {
                    auto ptr = ref_counted::decode_pointer<weak_value_type>(value);
                    ptr->sub_owner_ref();
                    return;
                }
            }
        }
    }


    template<class Derived, ref_counted_flags Flags, class CountType>
    inline auto ref_counted<Derived, Flags, CountType>::get_weak_value() const -> const weak_value_type *
    {
        static_assert(ref_counted::provides_weak_references, "class doesn't provide weak references");
        
        for (intptr_t value = this->m_count.load(std::memory_order_acquire); ; )
        {
            if (!ref_counted::is_encoded_pointer(value))
            {
                weak_reference<Derived> * ret = this->call_make_weak_reference(value);
                uintptr_t desired = ref_counted::encode_pointer(ret);
                if (this->m_count.compare_exchange_strong(value, desired, std::memory_order_release, std::memory_order_relaxed))
                    return ret;

                delete ret;
            }
            else
            {
                auto ptr = ref_counted::decode_pointer<weak_value_type>(value);
                ptr->add_ref();
                return ptr;
            }
        }
    }

    template<class Derived, ref_counted_flags Flags, class CountType>
    inline ref_counted<Derived, Flags, CountType>::~ref_counted() noexcept
    {
        [[maybe_unused]] auto valid_count = [](auto val) { return val == 0 || val == 1;};
        
        if constexpr (ref_counted::provides_weak_references)
        {
            intptr_t value = this->m_count.load(std::memory_order_relaxed);
            if (ref_counted::is_encoded_pointer(value))
            {
                auto ptr = ref_counted::decode_pointer<const weak_value_type>(value);
                assert(valid_count(ptr->m_strong.load(std::memory_order_relaxed)));
                ptr->call_on_owner_destruction();
                ptr->sub_ref();
            }
            else
            {
                assert(valid_count(value));
            }
        }
        else
        {
            assert(valid_count(this->m_count.load(std::memory_order_relaxed)));
        }
    }
}

#endif
