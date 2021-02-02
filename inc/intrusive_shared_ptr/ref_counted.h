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
    using default_count_type = std::conditional_t<contains(Flags, ref_counted_flags::provide_weak_references), intptr_t, int>;

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

    template<class Derived, ref_counted_flags Flags = ref_counted_flags::none>
    using weak_ref_counted_wrapper = ref_counted_wrapper<Derived, Flags | ref_counted_flags::provide_weak_references>;

    template<class Owner>
    class weak_reference;


    //MARK:-

    struct ref_counted_traits
    {
        template<class T>
        static void add_ref(const T * obj) noexcept
            { obj->call_add_ref(); }
        
        template<class T>
        static void sub_ref(const T * obj) noexcept
            { obj->call_sub_ref(); }
    };

    //MARK:-

    template<class Derived, ref_counted_flags Flags, class CountType>
    class ref_counted
    {
    template<class Owner> friend class weak_reference;
    friend ref_counted_traits;
    public:
        using refcnt_ptr_traits = ref_counted_traits;
        using ref_counted_base = ref_counted;
        
        static constexpr bool provides_weak_references = contains(Flags, ref_counted_flags::provide_weak_references);
        
    public:
        using weak_value_type   = std::conditional_t<ref_counted::provides_weak_references, weak_reference<Derived>, void>;
        using weak_ptr          = std::conditional_t<ref_counted::provides_weak_references, intrusive_shared_ptr<weak_value_type, ref_counted_traits>, void>;
        using const_weak_ptr    = std::conditional_t<ref_counted::provides_weak_references, intrusive_shared_ptr<const weak_value_type, ref_counted_traits>, void>;
        
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
            { return weak_ptr::noref(const_cast<weak_reference<X> *>(const_cast<const ref_counted *>(this)->call_get_weak_value())); }
        
        template<class X = Derived, class = std::enable_if_t<internal::dependent_bool<ref_counted::provides_weak_references, X>> >
        const_weak_ptr get_weak_ptr() const
            { return const_weak_ptr::noref(this->call_get_weak_value()); }
        
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
        //CRTP access
        void call_add_ref() const noexcept
            { static_cast<const Derived *>(this)->add_ref(); }
        void call_sub_ref() const noexcept
            { static_cast<const Derived *>(this)->sub_ref(); }
        
        void destroy() const noexcept
            { delete static_cast<const Derived *>(this); }

        auto call_make_weak_reference(intptr_t count) const
        {
            if constexpr (ref_counted::provides_weak_references)
                return static_cast<const Derived *>(this)->make_weak_reference(count);
        }
        
        auto call_get_weak_value() const
            { return static_cast<const Derived *>(this)->get_weak_value(); }
        
        //Weak reference pointer decoding and encoding
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

    template<class Owner>
    class weak_reference
    {
    template<class T, ref_counted_flags Flags, class CountType> friend class ref_counted;
    friend ref_counted_traits;
    public:
        using refcnt_ptr_traits = ref_counted_traits;
        using strong_value_type = Owner;
        using strong_ptr = intrusive_shared_ptr<strong_value_type, ref_counted_traits>;
        using const_strong_ptr = intrusive_shared_ptr<const strong_value_type, ref_counted_traits>;
        
    public:
        void add_ref() const noexcept;
        void sub_ref() const noexcept;
        
        template<class X = Owner>
        const_strong_ptr lock() const noexcept
            { return const_strong_ptr::noref(this->call_lock_owner()); }
        
        template<class X = Owner>
        strong_ptr lock() noexcept
            { return strong_ptr::noref(this->call_lock_owner()); }

    protected:
        constexpr weak_reference(intptr_t initial_strong, Owner * owner) noexcept:
            m_strong(initial_strong),
            m_owner(owner)
        {}

        ~weak_reference() noexcept = default;
        
        void add_owner_ref() noexcept;
        void sub_owner_ref() noexcept;
        
        strong_value_type * lock_owner() const noexcept;
        
        void on_owner_destruction() const noexcept
        {}
        
    private:
        template<class X=Owner>
        using derived_type = std::remove_pointer_t<decltype(std::declval<X>().call_make_weak_reference(0))>;
        
        void call_add_ref() const noexcept
            { static_cast<const derived_type<> *>(this)->add_ref(); }
        void call_sub_ref() const noexcept
            { static_cast<const derived_type<> *>(this)->sub_ref(); }
        void call_add_owner_ref() noexcept
            { static_cast<derived_type<> *>(this)->add_owner_ref(); }
        void call_sub_owner_ref() noexcept
            { static_cast<derived_type<> *>(this)->sub_owner_ref(); }
        void destroy() const
            { delete static_cast<const derived_type<> *>(this); }
        strong_value_type * call_lock_owner() const noexcept
            { return static_cast<const derived_type<> *>(this)->lock_owner(); }
        void call_on_owner_destruction() const noexcept
            { static_cast<const derived_type<> *>(this)->on_owner_destruction(); }
        
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

    template<class Owner>
    inline void weak_reference<Owner>::add_ref() const noexcept
    {
        [[maybe_unused]] auto oldcount = this->m_count.fetch_add(1, std::memory_order_relaxed);
        assert(oldcount > 0);
        assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
    }

    template<class Owner>
    inline void weak_reference<Owner>::sub_ref() const noexcept
    {
        auto oldcount = this->m_count.fetch_sub(1, std::memory_order_release);
        assert(oldcount > 0);
        if (oldcount == 1)
        {
            std::atomic_thread_fence(std::memory_order_acquire);
            this->destroy();
        }
    }

    template<class Owner>
    inline void weak_reference<Owner>::add_owner_ref() noexcept
    {
        [[maybe_unused]] auto oldcount = this->m_strong.fetch_add(1, std::memory_order_relaxed);
        assert(oldcount > 0);
        assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
    }

    template<class Owner>
    inline void weak_reference<Owner>::sub_owner_ref() noexcept
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

    template<class Owner>
    inline
    auto weak_reference<Owner>::lock_owner() const noexcept -> strong_value_type *
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
                    ptr->call_add_owner_ref();
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
                    ptr->call_sub_owner_ref();
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
                ptr->call_add_ref();
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
                ptr->call_sub_ref();
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
