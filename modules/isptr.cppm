/*
 Copyright 2024 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
*/
module;


#if (defined(__APPLE__) && defined(__MACH__))
    #include <CoreFoundation/CoreFoundation.h>
#endif

#if ISPTR_ENABLE_PYTHON
    #include <Python.h>
#endif

#if defined(_WIN32)
    #define NOMINMAX
    #include <Unknwn.h>
#endif

#include <atomic>
#include <cassert>
#include <compare>
#include <limits>
#include <memory>
#include <ostream>
#include <type_traits>
#include <utility>

export module isptr;

#define ISPTR_EXPORTED export 


#ifndef HEADER_INTRUSIVE_SHARED_PTR_H_INCLUDED
#define HEADER_INTRUSIVE_SHARED_PTR_H_INCLUDED


#ifndef HEADER_ISPTR_COMMON_H_INCLUDED
#define HEADER_ISPTR_COMMON_H_INCLUDED


#if __cpp_constexpr >= 201907L

    #define ISPTR_CONSTEXPR_SINCE_CPP20 constexpr

#else 

    #define ISPTR_CONSTEXPR_SINCE_CPP20

#endif

#if __cpp_impl_three_way_comparison >= 201907L


    #define ISPTR_USE_SPACESHIP_OPERATOR 1

#else 

    #define ISPTR_USE_SPACESHIP_OPERATOR 0

#endif

#if __cpp_lib_out_ptr >= 202106L
    

    #define ISPTR_SUPPORT_OUT_PTR 1

#else

    #define ISPTR_SUPPORT_OUT_PTR 0
    
#endif


#ifdef _MSC_VER

    #define ISPTR_ALWAYS_INLINE __forceinline
    #define ISPTR_TRIVIAL_ABI

#elif defined(__clang__) 

    #define ISPTR_ALWAYS_INLINE [[gnu::always_inline]] inline
    #define ISPTR_TRIVIAL_ABI [[clang::trivial_abi]]

#elif defined (__GNUC__)

    #define ISPTR_ALWAYS_INLINE [[gnu::always_inline]] inline
    #define ISPTR_TRIVIAL_ABI

#endif

#ifndef ISPTR_EXPORTED
    #define ISPTR_EXPORTED
#endif


namespace isptr::internal
{
    template<bool Val, class... Args>
    constexpr bool dependent_bool = Val;
}


#endif




namespace isptr
{

    namespace internal
    {

        struct add_ref_detector
        {
            template<class Traits, class T>
            auto operator()(Traits * , T * p) noexcept(noexcept(Traits::add_ref(p))) -> decltype(Traits::add_ref(p));
        };

        struct sub_ref_detector
        {
            template<class Traits, class T>
            auto operator()(Traits *, T * p) noexcept(noexcept(Traits::sub_ref(p))) -> decltype(Traits::sub_ref(p));
        };

    }

    template<class Traits, class T>
    constexpr bool are_intrusive_shared_traits = std::is_nothrow_invocable_v<internal::add_ref_detector, Traits *, T *> &&
                                                 std::is_nothrow_invocable_v<internal::sub_ref_detector, Traits *, T *>;


    ISPTR_EXPORTED
    template<class T, class Traits>
    class ISPTR_TRIVIAL_ABI intrusive_shared_ptr
    {
        static_assert(are_intrusive_shared_traits<Traits, T>, "Invalid Traits for type T");
        
        friend std::atomic<intrusive_shared_ptr<T, Traits>>;

    #if ISPTR_SUPPORT_OUT_PTR
        friend std::out_ptr_t<intrusive_shared_ptr<T, Traits>, T *>;
        friend std::inout_ptr_t<intrusive_shared_ptr<T, Traits>, T *>;
    #endif

    public:
        using pointer = T *;
        using element_type = T;
        using traits_type = Traits;
    private:
        class output_param
        {
            friend class intrusive_shared_ptr<T, Traits>;
        public:
            ISPTR_CONSTEXPR_SINCE_CPP20 ~output_param() noexcept
            { 
                m_owner->reset();
                m_owner->m_p = m_p;
            }
            
            constexpr operator T**() && noexcept
                { return &m_p; }

        private:
            constexpr output_param(intrusive_shared_ptr<T, Traits> & owner) noexcept :
                m_owner(&owner)
            {}
            constexpr output_param(output_param && src) noexcept = default;
            
            output_param(const output_param &) = delete;
            void operator=(const output_param &) = delete;
            void operator=(output_param &&) = delete;
        private:
            intrusive_shared_ptr<T, Traits> * m_owner;
            T * m_p = nullptr;
        };
    public:
        static constexpr intrusive_shared_ptr noref(T * p) noexcept
            { return intrusive_shared_ptr(p); }
        static constexpr intrusive_shared_ptr ref(T * p) noexcept
        {
            intrusive_shared_ptr::do_add_ref(p);
            return intrusive_shared_ptr(p);
        }
        
        constexpr intrusive_shared_ptr() noexcept : m_p(nullptr)
            {}
        constexpr intrusive_shared_ptr(std::nullptr_t) noexcept : m_p(nullptr)
            {}
        constexpr intrusive_shared_ptr(const intrusive_shared_ptr<T, Traits> & src) noexcept : m_p(src.m_p)
            { this->do_add_ref(this->m_p); }
        constexpr intrusive_shared_ptr(intrusive_shared_ptr<T, Traits> && src) noexcept : m_p(src.release())
            { }
        constexpr intrusive_shared_ptr<T, Traits> & operator=(const intrusive_shared_ptr<T, Traits> & src) noexcept
        {
            T * temp = this->m_p;
            this->m_p = src.m_p;
            this->do_add_ref(this->m_p);
            this->do_sub_ref(temp);
            return *this;
        }
        constexpr intrusive_shared_ptr<T, Traits> & operator=(intrusive_shared_ptr<T, Traits> && src) noexcept
        {
            T * new_val = src.release();
            //this must come second so it is nullptr if src is us
            T * old_val = this->m_p;
            this->m_p = new_val;
            this->do_sub_ref(old_val);
            return *this;
        }
        
        template<class Y, class YTraits, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr(const intrusive_shared_ptr<Y, YTraits> & src) noexcept : m_p(src.get())
            { this->do_add_ref(this->m_p); }
        template<class Y, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr(intrusive_shared_ptr<Y, Traits> && src) noexcept : m_p(src.release())
            {}
        template<class Y, class YTraits, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr(intrusive_shared_ptr<Y, YTraits> && src) noexcept : m_p(src.get())
        {
            this->do_add_ref(this->m_p);
            src.reset();
        }
        template<class Y, class YTraits, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr<T, Traits> & operator=(const intrusive_shared_ptr<Y, YTraits> & src) noexcept
        {
            T * temp = this->m_p;
            this->m_p = src.get();
            this->do_add_ref(this->m_p);
            this->do_sub_ref(temp);
            return *this;
        }
        template<class Y, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr<T, Traits> & operator=(intrusive_shared_ptr<Y, Traits> && src) noexcept
        {
            this->do_sub_ref(this->m_p);
            this->m_p = src.release();
            return *this;
        }
        template<class Y, class YTraits, class = std::enable_if_t<std::is_convertible_v<Y *, T *>, void>>
        constexpr intrusive_shared_ptr<T, Traits> & operator=(intrusive_shared_ptr<Y, YTraits> && src) noexcept
        {
            this->do_sub_ref(this->m_p);
            this->m_p = src.get();
            this->do_add_ref(this->m_p);
            src.reset();
            return *this;
        }
        
        
        ISPTR_CONSTEXPR_SINCE_CPP20 ~intrusive_shared_ptr() noexcept
            { this->reset(); }

        constexpr T * get() const noexcept
            { return this->m_p; }
        
        constexpr T * operator->() const noexcept
            { return this->m_p; }
        
        template<class X=T>
        constexpr
        std::enable_if_t<std::is_same_v<X, T>,
        X &> operator*() const noexcept
            { return *this->m_p; }
        
        template<class M, class X=T>
        constexpr
        std::enable_if_t<std::is_same_v<X, T>,
        M &> operator->*(M X::*memptr) const noexcept
            { return this->m_p->*memptr; }
        
        constexpr explicit operator bool() const noexcept
            { return this->m_p; }

        constexpr output_param get_output_param() noexcept
            { return output_param(*this); }
        
        constexpr T * release() noexcept
        { 
            T * p = this->m_p;
            this->m_p = nullptr;
            return p;
        }

        ISPTR_ALWAYS_INLINE //GCC refuses to inline this otherwise
        constexpr void reset() noexcept
        { 
            this->do_sub_ref(this->m_p);
            this->m_p = nullptr;
        }
        
        constexpr void swap(intrusive_shared_ptr<T, Traits> & other) noexcept
        {
            T * temp = this->m_p;
            this->m_p = other.m_p;
            other.m_p = temp;
        }

        friend constexpr void swap(intrusive_shared_ptr<T, Traits> & lhs, intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            lhs.swap(rhs);
        }

        template<class Y, class YTraits>
        friend constexpr bool operator==(const intrusive_shared_ptr<T, Traits>& lhs, const intrusive_shared_ptr<Y, YTraits>& rhs) noexcept
        {
            return lhs.m_p == rhs.get();
        }

        template<class Y>
        friend constexpr bool operator==(const intrusive_shared_ptr<T, Traits>& lhs, const Y* rhs) noexcept
        {
            return lhs.m_p == rhs;
        }

        template<class Y>
        friend constexpr bool operator==(const Y* lhs, const intrusive_shared_ptr<T, Traits>& rhs) noexcept
        {
            return lhs == rhs.m_p;
        }

        friend constexpr bool operator==(const intrusive_shared_ptr<T, Traits>& lhs, std::nullptr_t) noexcept
        {
            return lhs.m_p == nullptr;
        }

        friend constexpr bool operator==(std::nullptr_t, const intrusive_shared_ptr<T, Traits>& rhs) noexcept
        {
            return nullptr == rhs.m_p;
        }

        template<class Y, class YTraits>
        friend constexpr bool operator!=(const intrusive_shared_ptr<T, Traits>& lhs, const intrusive_shared_ptr<Y, YTraits>& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        template<class Y>
        friend constexpr bool operator!=(const intrusive_shared_ptr<T, Traits>& lhs, const Y* rhs) noexcept
        {
            return !(lhs == rhs);
        }

        template<class Y>
        friend constexpr bool operator!=(const Y* lhs, const intrusive_shared_ptr<T, Traits>& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend constexpr bool operator!=(const intrusive_shared_ptr<T, Traits>& lhs, std::nullptr_t) noexcept
        {
            return !(lhs == nullptr);
        }

        friend constexpr bool operator!=(std::nullptr_t, const intrusive_shared_ptr<T, Traits>& rhs) noexcept
        {
            return !(nullptr == rhs);
        }

    #if ISPTR_USE_SPACESHIP_OPERATOR

        template<class Y, class YTraits>
        friend constexpr auto operator<=>(const intrusive_shared_ptr<T, Traits> & lhs, const intrusive_shared_ptr<Y, YTraits> & rhs) noexcept
        {
            return lhs.m_p <=> rhs.get();
        }

        template<class Y>
        friend constexpr auto operator<=>(const intrusive_shared_ptr<T, Traits> & lhs, const Y * rhs) noexcept
        {
            return lhs.m_p <=> rhs;
        }

        template<class Y>
        friend constexpr auto operator<=>(const Y * lhs, const intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            return lhs <=> rhs.m_p;
        }

    #else
        template<class Y, class YTraits>
        friend constexpr bool operator<(const intrusive_shared_ptr<T, Traits> & lhs, const intrusive_shared_ptr<Y, YTraits> & rhs) noexcept
        {
            return lhs.m_p < rhs.get();
        }

        template<class Y>
        friend constexpr bool operator<(const intrusive_shared_ptr<T, Traits> & lhs, const Y * rhs) noexcept
        {
            return lhs.m_p < rhs;
        }

        template<class Y>
        friend constexpr bool operator<(const Y * lhs, const intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            return lhs < rhs.m_p;
        }

        template<class Y, class YTraits>
        friend constexpr bool operator<=(const intrusive_shared_ptr<T, Traits> & lhs, const intrusive_shared_ptr<Y, YTraits> & rhs) noexcept
        {
            return lhs.m_p <= rhs.get();
        }

        template<class Y>
        friend constexpr bool operator<=(const intrusive_shared_ptr<T, Traits> & lhs, const Y * rhs) noexcept
        {
            return lhs.m_p <= rhs;
        }

        template<class Y>
        friend constexpr bool operator<=(const Y * lhs, const intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            return lhs <= rhs.m_p;
        }

        template<class Y, class YTraits>
        friend constexpr bool operator>(const intrusive_shared_ptr<T, Traits> & lhs, const intrusive_shared_ptr<Y, YTraits> & rhs) noexcept
        {
            return !(lhs <= rhs);
        }

        template<class Y>
        friend constexpr bool operator>(const intrusive_shared_ptr<T, Traits> & lhs, const Y * rhs) noexcept
        {
            return !(lhs <= rhs);
        }

        template<class Y>
        friend constexpr bool operator>(const Y * lhs, const intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            return !(lhs <= rhs);
        }

        template<class Y, class YTraits>
        friend constexpr bool operator>=(const intrusive_shared_ptr<T, Traits> & lhs, const intrusive_shared_ptr<Y, YTraits> & rhs) noexcept
        {
            return !(lhs < rhs);
        }

        template<class Y>
        friend constexpr bool operator>=(const intrusive_shared_ptr<T, Traits> & lhs, const Y * rhs) noexcept
        {
            return !(lhs < rhs);
        }

        template<class Y>
        friend constexpr bool operator>=(const Y * lhs, const intrusive_shared_ptr<T, Traits> & rhs) noexcept
        {
            return !(lhs < rhs);
        }

        

    #endif

        template<class Char>
        friend std::basic_ostream<Char> & operator<<(std::basic_ostream<Char> & str, const intrusive_shared_ptr<T, Traits> & ptr)
        {
            return str << ptr.m_p;
        }
        
    private:
        constexpr intrusive_shared_ptr(T * ptr) noexcept :
            m_p(ptr)
        {
        }

        static constexpr void do_add_ref(T * p) noexcept
        {
            if (p) Traits::add_ref(p);
        }
        static constexpr void do_sub_ref(T * p) noexcept
        {
            if (p) Traits::sub_ref(p);
        }
    private:
        T * m_p;
    };

    template<class T>
    std::false_type is_intrusive_shared_ptr_helper(const T &);

    template<class T, class Traits>
    std::true_type is_intrusive_shared_ptr_helper(const intrusive_shared_ptr<T, Traits> &);

    template<class T>
    using is_intrusive_shared_ptr = decltype(is_intrusive_shared_ptr_helper(std::declval<T>()));

    template<class T>
    bool constexpr is_intrusive_shared_ptr_v = is_intrusive_shared_ptr<T>::value;



    ISPTR_EXPORTED
    template<class Dest, class Src, class Traits>
    inline constexpr
    std::enable_if_t<is_intrusive_shared_ptr_v<Dest>,
    Dest> intrusive_const_cast(intrusive_shared_ptr<Src, Traits> p) noexcept
    {
        return Dest::noref(const_cast<typename Dest::pointer>(p.release()));
    }

    ISPTR_EXPORTED
    template<class Dest, class Src, class Traits>
    inline constexpr
    std::enable_if_t<is_intrusive_shared_ptr_v<Dest>,
    Dest> intrusive_dynamic_cast(intrusive_shared_ptr<Src, Traits> p) noexcept
    {
        auto res = dynamic_cast<typename Dest::pointer>(p.get());
        if (res)
        {
            p.release();
            return Dest::noref(res);
        }
        return Dest();
    }

    ISPTR_EXPORTED
    template<class Dest, class Src, class Traits>
    inline constexpr
    std::enable_if_t<is_intrusive_shared_ptr_v<Dest>,
    Dest> intrusive_static_cast(intrusive_shared_ptr<Src, Traits> p) noexcept
    {
        return Dest::noref(static_cast<typename Dest::pointer>(p.release()));
    }
}

namespace std
{
    ISPTR_EXPORTED
    template<class Traits, class T>
    class atomic<isptr::intrusive_shared_ptr<T, Traits>>
    {
    public:
        using value_type = isptr::intrusive_shared_ptr<T, Traits>;
    public:
        static constexpr bool is_always_lock_free = std::atomic<T *>::is_always_lock_free;

        constexpr atomic() noexcept = default;
        atomic(value_type desired) noexcept : m_p(desired.m_p)
        {
            desired.m_p = nullptr;
        }
        
        atomic(const atomic&) = delete;
        void operator=(const atomic&) = delete;
        
        ~atomic() noexcept
        {
            value_type::do_sub_ref(this->m_p.load(memory_order_acquire));
        }

        void operator=(value_type desired) noexcept
        {
            this->store(std::move(desired));
        }

        operator value_type() const noexcept
        {
            return this->load();
        }

        value_type load(memory_order order = memory_order_seq_cst) const noexcept
        {
            T * ret = this->m_p.load(order);
            return value_type::ref(ret);
        }

        void store(value_type desired, memory_order order = memory_order_seq_cst) noexcept
        {
            exchange(std::move(desired), order);
        }
        
        value_type exchange(value_type desired, memory_order order = memory_order_seq_cst) noexcept
        {
            T * ret = this->m_p.exchange(desired.m_p, order);
            desired.m_p = nullptr;
            return value_type::noref(ret);
        }

        bool compare_exchange_strong(value_type & expected, value_type desired, memory_order success, memory_order failure) noexcept
        {
            T * saved_expected = expected.m_p;

            bool ret = this->m_p.compare_exchange_strong(expected.m_p, desired.m_p, success, failure);
            return post_compare_exchange(ret, saved_expected, expected, desired);
        }
        bool compare_exchange_strong(value_type & expected, value_type desired, memory_order order = memory_order_seq_cst) noexcept
        {
            T * saved_expected = expected.m_p;

            bool ret = this->m_p.compare_exchange_strong(expected.m_p, desired.m_p, order);
            return post_compare_exchange(ret, saved_expected, expected, desired);
        }

        bool compare_exchange_weak(value_type & expected, value_type desired, memory_order success, memory_order failure) noexcept
        {
            T * saved_expected = expected.m_p;

            bool ret = this->m_p.compare_exchange_weak(expected.m_p, desired.m_p, success, failure);
            return post_compare_exchange(ret, saved_expected, expected, desired);
        }
        bool compare_exchange_weak(value_type & expected, value_type desired, memory_order order = memory_order_seq_cst) noexcept
        {
            T * saved_expected = expected.m_p;

            bool ret = this->m_p.compare_exchange_weak(expected.m_p, desired.m_p, order);
            return post_compare_exchange(ret, saved_expected, expected, desired);
        }


        bool is_lock_free() const noexcept
            { return this->m_p.is_lock_free(); }
        
    private:
        static bool post_compare_exchange(bool exchange_result, T * saved_expected, 
                                          value_type & expected, value_type & desired) noexcept
        {
            if (exchange_result)
            {
                //success: we are desired and expected is unchanged
                desired.m_p = nullptr;
                //saved_expected is equal to our original value which we need to sub_ref
                value_type::do_sub_ref(saved_expected); 
            }
            else
            {
                //failure: expected is us and desired is unchanged. 
                value_type::do_add_ref(expected.m_p); //our value going out
                value_type::do_sub_ref(saved_expected); //old expected
            }
            return exchange_result;
        }
    private:
        std::atomic<T *> m_p = nullptr;
    };

#if ISPTR_SUPPORT_OUT_PTR

    ISPTR_EXPORTED
    template<class T, class Traits>
    class out_ptr_t<isptr::intrusive_shared_ptr<T, Traits>, T *>
    {
    public:
        constexpr out_ptr_t(isptr::intrusive_shared_ptr<T, Traits> & owner) noexcept :
            m_owner(&owner)
        {}
        constexpr out_ptr_t(out_ptr_t && src) noexcept = default;
        out_ptr_t(const out_ptr_t &) = delete;

        void operator=(const out_ptr_t &) = delete;
        void operator=(out_ptr_t &&) = delete;

        ISPTR_CONSTEXPR_SINCE_CPP20 ~out_ptr_t() noexcept
        { 
            m_owner->reset();
            m_owner->m_p = m_p;
        }
        
        constexpr operator T**() const noexcept
            { return const_cast<T **>(&m_p); }

        constexpr operator void**() const noexcept requires(!std::is_same_v<T *, void *>)
            { return reinterpret_cast<void**>(const_cast<T **>(&m_p)); }
    private:
        isptr::intrusive_shared_ptr<T, Traits> * m_owner;
        T * m_p = nullptr;
    };

    ISPTR_EXPORTED
    template<class T, class Traits>
    class inout_ptr_t<isptr::intrusive_shared_ptr<T, Traits>, T *>
    {
    public:
        constexpr inout_ptr_t(isptr::intrusive_shared_ptr<T, Traits> & owner) noexcept :
            m_owner(&owner),
            m_p(std::exchange(owner.m_p, nullptr))
        {}
        constexpr inout_ptr_t(inout_ptr_t && src) noexcept = default;
        inout_ptr_t(const inout_ptr_t &) = delete;

        void operator=(const inout_ptr_t &) = delete;
        void operator=(inout_ptr_t &&) = delete;

        ISPTR_CONSTEXPR_SINCE_CPP20 ~inout_ptr_t() noexcept
            { m_owner->m_p = m_p; }
        
        constexpr operator T**() const noexcept
            { return const_cast<T **>(&m_p); }

        constexpr operator void**() const noexcept requires(!std::is_same_v<T *, void *>)
            { return reinterpret_cast<void**>(const_cast<T **>(&m_p)); }
    private:
        isptr::intrusive_shared_ptr<T, Traits> * m_owner;
        T * m_p;
    };

#endif
}

#undef ISPTR_TRIVIAL_ABI
#undef ISPTR_CONSTEXPR_SINCE_CPP20
#undef ISPTR_USE_SPACESHIP_OPERATOR

#endif


#ifndef HEADER_REF_COUNTED_H_INCLUDED
#define HEADER_REF_COUNTED_H_INCLUDED



namespace isptr
{

    //MARK:- ref_counted_flags

    ISPTR_EXPORTED
    enum class ref_counted_flags : unsigned
    {
        none = 0,
        provide_weak_references = 1,
        single_threaded = 2
    };

    ISPTR_EXPORTED constexpr ref_counted_flags operator|(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) | unsigned(rhs)); }
    ISPTR_EXPORTED constexpr ref_counted_flags operator&(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) & unsigned(rhs)); }
    ISPTR_EXPORTED constexpr ref_counted_flags operator^(ref_counted_flags lhs, ref_counted_flags rhs) noexcept
        { return ref_counted_flags(unsigned(lhs) ^ unsigned(rhs)); }
    ISPTR_EXPORTED constexpr ref_counted_flags operator~(ref_counted_flags arg) noexcept
        { return ref_counted_flags(~unsigned(arg)); }

    ISPTR_EXPORTED constexpr bool contains(ref_counted_flags val, ref_counted_flags flag) noexcept
        { return (val & flag) == flag;   }

    //MARK:- Forward Declarations

    ISPTR_EXPORTED
    template<ref_counted_flags Flags>
    using default_count_type = std::conditional_t<contains(Flags, ref_counted_flags::provide_weak_references), intptr_t, int>;

    ISPTR_EXPORTED
    template<class Derived, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted;

    ISPTR_EXPORTED
    template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted_adapter;

    ISPTR_EXPORTED
    template<class T, ref_counted_flags Flags = ref_counted_flags::none, class CountType = default_count_type<Flags>>
    class ref_counted_wrapper;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted = ref_counted<Derived, ref_counted_flags::provide_weak_references>;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted_adapter = ref_counted_adapter<Derived, ref_counted_flags::provide_weak_references>;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted_wrapper = ref_counted_wrapper<Derived, ref_counted_flags::provide_weak_references>;

    ISPTR_EXPORTED
    template<class Derived, class CountType = default_count_type<ref_counted_flags::single_threaded>>
    using ref_counted_st = ref_counted<Derived, ref_counted_flags::single_threaded, CountType>;

    ISPTR_EXPORTED
    template<class Derived, class CountType = default_count_type<ref_counted_flags::single_threaded>>
    using ref_counted_adapter_st = ref_counted_adapter<Derived, ref_counted_flags::single_threaded, CountType>;

    ISPTR_EXPORTED
    template<class Derived, class CountType = default_count_type<ref_counted_flags::single_threaded>>
    using ref_counted_wrapper_st = ref_counted_wrapper<Derived, ref_counted_flags::single_threaded, CountType>;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted_st = ref_counted<Derived, ref_counted_flags::provide_weak_references | 
                                                     ref_counted_flags::single_threaded>;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted_adapter_st = ref_counted_adapter<Derived, ref_counted_flags::provide_weak_references | 
                                                                     ref_counted_flags::single_threaded>;

    ISPTR_EXPORTED
    template<class Derived>
    using weak_ref_counted_wrapper_st = ref_counted_wrapper<Derived, ref_counted_flags::provide_weak_references |
                                                                     ref_counted_flags::single_threaded>;

    ISPTR_EXPORTED
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
        static constexpr bool single_threaded = contains(Flags, ref_counted_flags::single_threaded);
        
    public:
        using weak_value_type   = std::conditional_t<ref_counted::provides_weak_references, weak_reference<Derived>, void>;
        using weak_ptr          = std::conditional_t<ref_counted::provides_weak_references, intrusive_shared_ptr<weak_value_type, ref_counted_traits>, void>;
        using const_weak_ptr    = std::conditional_t<ref_counted::provides_weak_references, intrusive_shared_ptr<const weak_value_type, ref_counted_traits>, void>;
        
    private:
        static_assert(!ref_counted::provides_weak_references || (ref_counted::provides_weak_references && std::is_same_v<CountType, intptr_t>),
                      "CountType must be intptr_t (the default) when providing weak references");
        static_assert(std::is_integral_v<CountType>, "CountType must be an integral type");
        static_assert(ref_counted::single_threaded || std::atomic<CountType>::is_always_lock_free,
                      "CountType must be such that std::atomic<CountType> is alwayd lock free");
        
        using count_type = std::conditional_t<ref_counted::single_threaded, CountType, std::atomic<CountType>>;

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
        
        void destroy() const noexcept
            { delete static_cast<const Derived *>(this); }
        
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
        
        void call_destroy() const noexcept
            { static_cast<const Derived *>(this)->destroy(); }

        auto call_make_weak_reference(intptr_t count) const
        {
            if constexpr (ref_counted::provides_weak_references)
                return static_cast<const Derived *>(this)->make_weak_reference(count);
        }
        
        auto call_get_weak_value() const
            { return static_cast<const Derived *>(this)->get_weak_value(); }
        
        //Weak reference pointer decoding and encoding
        template<class X>
        static X * decode_pointer(intptr_t count) noexcept
            { return (X *)(uintptr_t(count) << 1); }

        template<class X>
        static intptr_t encode_pointer(X * ptr) noexcept
            { return (uintptr_t(ptr) >> 1) | uintptr_t(std::numeric_limits<intptr_t>::min()); }

        static bool is_encoded_pointer(intptr_t count) noexcept
            { return count < 0; }
    private:
        mutable count_type m_count = 1;
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

        static constexpr bool single_threaded = Owner::single_threaded;

    private:
        using count_type = std::conditional_t<weak_reference::single_threaded, intptr_t, std::atomic<intptr_t>>;
        
    public:
        weak_reference(const weak_reference &) noexcept = delete;
        weak_reference & operator=(const weak_reference &) noexcept = delete;
        weak_reference(weak_reference &&) noexcept = delete;
        weak_reference & operator=(weak_reference &&) noexcept = delete;

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
        
        void destroy() const
            { delete static_cast<const derived_type<> *>(this); }
        
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
        void call_destroy() const
            { static_cast<const derived_type<> *>(this)->destroy(); }
        strong_value_type * call_lock_owner() const noexcept
            { return static_cast<const derived_type<> *>(this)->lock_owner(); }
        void call_on_owner_destruction() const noexcept
            { static_cast<const derived_type<> *>(this)->on_owner_destruction(); }
        
    private:
        mutable count_type m_count = 2;
        mutable count_type m_strong = 0;
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
        if constexpr (!weak_reference::single_threaded) 
        {
            [[maybe_unused]] auto oldcount = this->m_count.fetch_add(1, std::memory_order_relaxed);
            assert(oldcount > 0);
            assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
        } 
        else 
        {
            assert(this->m_count > 0);
            assert(this->m_count < std::numeric_limits<decltype(this->m_count)>::max());
            ++this->m_count;
        }
    }

    template<class Owner>
    inline void weak_reference<Owner>::sub_ref() const noexcept
    {
        if constexpr (!weak_reference::single_threaded) 
        {
            auto oldcount = this->m_count.fetch_sub(1, std::memory_order_release);
            assert(oldcount > 0);
            if (oldcount == 1)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                this->call_destroy();
            }
        }
        else 
        {
            assert(this->m_count > 0);
            if (--this->m_count == 0)
                this->call_destroy();
        }
    }

    template<class Owner>
    inline void weak_reference<Owner>::add_owner_ref() noexcept
    {
        if constexpr (!weak_reference::single_threaded) 
        {
            [[maybe_unused]] auto oldcount = this->m_strong.fetch_add(1, std::memory_order_relaxed);
            assert(oldcount > 0);
            assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
        }
        else 
        {
            assert(this->m_strong > 0);
            assert(this->m_strong < std::numeric_limits<decltype(this->m_count)>::max());
            ++this->m_strong;
        }
    }

    template<class Owner>
    inline void weak_reference<Owner>::sub_owner_ref() noexcept
    {
        if constexpr (!weak_reference::single_threaded) 
        {
            auto oldcount = this->m_strong.fetch_sub(1, std::memory_order_release);
            assert(oldcount > 0);
            if (oldcount == 1)
            {
                std::atomic_thread_fence(std::memory_order_acquire);
                auto owner = this->m_owner;
                this->m_owner = nullptr;
                owner->call_destroy(); //this can cascade to deleting ourselves so must be the last thing
            }
        } 
        else 
        {
            assert(this->m_strong > 0);
            if (--this->m_strong == 0) 
            {
                auto owner = this->m_owner;
                this->m_owner = nullptr;
                owner->call_destroy(); //this can cascade to deleting ourselves so must be the last thing
            }
        }
    }

    template<class Owner>
    inline
    auto weak_reference<Owner>::lock_owner() const noexcept -> strong_value_type *
    {
        if constexpr (!weak_reference::single_threaded) 
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
        else
        {
            if (this->m_strong == 0)
                return nullptr;
            ++this->m_strong;
            return this->m_owner;
        }
    }

    template<class Derived, ref_counted_flags Flags, class CountType>
    inline void ref_counted<Derived, Flags, CountType>::add_ref() const noexcept
    {
        if constexpr(!ref_counted::provides_weak_references)
        {
            if constexpr(!ref_counted::single_threaded)
            {
                [[maybe_unused]] auto oldcount = this->m_count.fetch_add(1, std::memory_order_relaxed);
                assert(oldcount > 0);
                assert(oldcount < std::numeric_limits<decltype(oldcount)>::max());
            }
            else
            {
                assert(this->m_count > 0);
                assert(this->m_count < std::numeric_limits<decltype(this->m_count)>::max());
                ++this->m_count;
            }
        }
        else
        {
            if constexpr(!ref_counted::single_threaded)
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
            else 
            {
                assert(this->m_count != 0);
                if (!ref_counted::is_encoded_pointer(this->m_count))
                {
                    assert(this->m_count < std::numeric_limits<decltype(this->m_count)>::max());
                    ++this->m_count;
                }
                else 
                {
                    auto ptr = ref_counted::decode_pointer<weak_value_type>(this->m_count);
                    ptr->call_add_owner_ref();
                }
            }
        }
    }
        
    template<class Derived, ref_counted_flags Flags, class CountType>
    inline void ref_counted<Derived, Flags, CountType>::sub_ref() const noexcept
    {
        if constexpr(!ref_counted::provides_weak_references)
        {
            if constexpr(!ref_counted::single_threaded)
            {
                auto oldcount = this->m_count.fetch_sub(1, std::memory_order_release);
                assert(oldcount > 0);
                if (oldcount == 1)
                {
                    std::atomic_thread_fence(std::memory_order_acquire);
                    this->call_destroy();
                }
            }
            else
            {
                assert(this->m_count > 0);
                if (--this->m_count == 0)
                    this->call_destroy();
            }
        }
        else
        {
            if constexpr(!ref_counted::single_threaded)
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
                                this->call_destroy();
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
            else
            {
                assert(this->m_count != 0);
                if (!ref_counted::is_encoded_pointer(this->m_count))
                {
                    if (--this->m_count == 0)
                        this->call_destroy();
                }
                else
                {
                    auto ptr = ref_counted::decode_pointer<weak_value_type>(this->m_count);
                    ptr->call_sub_owner_ref();
                }
            }
        }
    }


    template<class Derived, ref_counted_flags Flags, class CountType>
    inline auto ref_counted<Derived, Flags, CountType>::get_weak_value() const -> const weak_value_type *
    {
        static_assert(ref_counted::provides_weak_references, "class doesn't provide weak references");
        
        if constexpr(!ref_counted::single_threaded)
        {
            for (intptr_t value = this->m_count.load(std::memory_order_acquire); ; )
            {
                if (!ref_counted::is_encoded_pointer(value))
                {
                    weak_reference<Derived> * ret = this->call_make_weak_reference(value);
                    uintptr_t desired = ref_counted::encode_pointer(ret);
                    if (this->m_count.compare_exchange_strong(value, desired, std::memory_order_release, std::memory_order_relaxed))
                        return ret;

                    ret->call_destroy();
                }
                else
                {
                    auto ptr = ref_counted::decode_pointer<weak_value_type>(value);
                    ptr->call_add_ref();
                    return ptr;
                }
            }
        }
        else
        {
            if (!ref_counted::is_encoded_pointer(this->m_count))
            {
                weak_reference<Derived> * ret = this->call_make_weak_reference(this->m_count);
                this->m_count = ref_counted::encode_pointer(ret);
                return ret;
            }
            else 
            {
                auto ptr = ref_counted::decode_pointer<weak_value_type>(this->m_count);
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
            if constexpr(!ref_counted::single_threaded)
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
                if (ref_counted::is_encoded_pointer(this->m_count))
                {
                    auto ptr = ref_counted::decode_pointer<const weak_value_type>(this->m_count);
                    assert(valid_count(ptr->m_strong));
                    ptr->call_on_owner_destruction();
                    ptr->call_sub_ref();
                }
                else
                {
                    assert(valid_count(this->m_count));
                }
            }
        }
        else
        {
            if constexpr(!ref_counted::single_threaded)
                assert(valid_count(this->m_count.load(std::memory_order_relaxed)));
            else
                assert(valid_count(this->m_count));
        }
    }
}

#endif

#ifndef HEADER_APPLE_CF_PTR_H_INCLUDED
#define HEADER_APPLE_CF_PTR_H_INCLUDED

#if (defined(__APPLE__) && defined(__MACH__))



namespace isptr
{
    struct cf_traits
    {
        static void add_ref(CFTypeRef ptr) noexcept
            { CFRetain(ptr); }
        static void sub_ref(CFTypeRef ptr) noexcept
            { CFRelease(ptr); }
    };

    ISPTR_EXPORTED
    template<class T>
    using cf_ptr = intrusive_shared_ptr<std::remove_pointer_t<T>, cf_traits>;

    ISPTR_EXPORTED
    template<class T>
    cf_ptr<T *> cf_retain(T * ptr) {
        return cf_ptr<T *>::ref(ptr);
    }
    ISPTR_EXPORTED
    template<class T>
    cf_ptr<T *> cf_attach(T * ptr) {
        return cf_ptr<T *>::noref(ptr);
    }
}

#endif

#endif

#ifndef HEADER_COM_PTR_H_INCLUDED
#define HEADER_COM_PTR_H_INCLUDED

#if defined(_WIN32)



namespace isptr
{
    struct com_traits
    {
        template<class T>
        static std::enable_if_t<std::is_base_of_v<IUnknown, T>, void> add_ref(T * ptr) noexcept
            { ptr->AddRef(); }
        template<class T>
        static std::enable_if_t<std::is_base_of_v<IUnknown, T>, void> sub_ref(T * ptr) noexcept
            { ptr->Release(); }
    };

    ISPTR_EXPORTED
    template<class T>
    using com_shared_ptr = intrusive_shared_ptr<T, com_traits>;

    ISPTR_EXPORTED
    template<class T>
    com_shared_ptr<T> com_retain(T * ptr) {
        return com_shared_ptr<T>::ref(ptr);
    }
    ISPTR_EXPORTED
    template<class T>
    com_shared_ptr<T> com_attach(T * ptr) {
        return com_shared_ptr<T>::noref(ptr);
    }
}

#endif

#endif
#if ISPTR_ENABLE_PYTHON

#ifndef HEADER_PYTHON_PTR_H_INCLUDED
#define HEADER_PYTHON_PTR_H_INCLUDED



namespace isptr
{
    struct py_traits {
        static void add_ref(PyObject * ptr) noexcept
            { Py_INCREF(ptr); }
        static void sub_ref(PyObject * ptr) noexcept
            { Py_DECREF(ptr); }

        static void add_ref(PyTypeObject * ptr) noexcept
            { Py_INCREF(ptr); }
        static void sub_ref(PyTypeObject * ptr) noexcept
            { Py_DECREF(ptr); }
    };

    template<class T>
    using py_ptr = intrusive_shared_ptr<T, py_traits>;

    ISPTR_EXPORTED
    template<class T>
    py_ptr<T> py_retain(T * ptr) {
        return py_ptr<T>::ref(ptr);
    }
    ISPTR_EXPORTED
    template<class T>
    py_ptr<T> py_attach(T * ptr) {
        return py_ptr<T>::noref(ptr);
    }
}

#endif
#endif

#ifndef HEADER_REFCNT_PTR_H_INCLUDED
#define HEADER_REFCNT_PTR_H_INCLUDED


namespace isptr
{

    ISPTR_EXPORTED
    template<class T>
    using refcnt_ptr = intrusive_shared_ptr<T, typename T::refcnt_ptr_traits>;

    ISPTR_EXPORTED
    template<class T>
    constexpr refcnt_ptr<T> refcnt_retain(T * ptr) noexcept {
        return refcnt_ptr<T>::ref(ptr);
    }

    ISPTR_EXPORTED
    template<class T>
    constexpr refcnt_ptr<T> refcnt_attach(T * ptr) noexcept {
        return refcnt_ptr<T>::noref(ptr);
    }

    ISPTR_EXPORTED
    template<class T, class... Args>
    inline refcnt_ptr<T> make_refcnt(Args &&... args) {
        return refcnt_ptr<T>::noref(new T( std::forward<Args>(args)... ));
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<typename T::weak_value_type> weak_cast(const refcnt_ptr<T> & src) {
        return src->get_weak_ptr();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<const typename T::weak_value_type> weak_cast(const refcnt_ptr<const T> & src) {
        return src->get_weak_ptr();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<typename T::strong_value_type> strong_cast(const refcnt_ptr<T> & src) noexcept {
        return src->lock();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<const typename T::strong_value_type> strong_cast(const refcnt_ptr<const T> & src) noexcept {
        return src->lock();
    }
}

#endif

