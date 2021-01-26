#ifndef HEADER_INTRUSIVE_SHARED_PTR_H_INCLUDED
#define HEADER_INTRUSIVE_SHARED_PTR_H_INCLUDED

#include <type_traits>
#include <ostream>
#include <atomic>


#if __cpp_constexpr >= 201907L

    #define INTRUSIVE_SHARED_PTR_CONSTEXPR_SINCE_CPP20 constexpr

#else 

    #define INTRUSIVE_SHARED_PTR_CONSTEXPR_SINCE_CPP20

#endif

#if __cpp_impl_three_way_comparison >= 201907L

    #include <compare>

    #define INTRUSIVE_SHARED_PTR_USE_SPACESHIP_OPERATOR 1

#else 

    #define INTRUSIVE_SHARED_PTR_USE_SPACESHIP_OPERATOR 0

#endif

#ifdef __clang__
    #define INTRUSIVE_SHARED_PTR_TRIVIAL_ABI [[clang::trivial_abi]]
#else
    #define INTRUSIVE_SHARED_PTR_TRIVIAL_ABI
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


    template<class T, class Traits>
    class INTRUSIVE_SHARED_PTR_TRIVIAL_ABI intrusive_shared_ptr
    {
        static_assert(are_intrusive_shared_traits<Traits, T>, "Invalid Traits for type T");
        
        friend std::atomic<intrusive_shared_ptr<T, Traits>>;
    public:
        using pointer = T *;
        using element_type = T;
        using traits_type = Traits;
    private:
        class output_param
        {
            friend class intrusive_shared_ptr<T, Traits>;
        public:
            INTRUSIVE_SHARED_PTR_CONSTEXPR_SINCE_CPP20 ~output_param() noexcept
            {
                if (m_p != m_owner->get())
                    *m_owner = intrusive_shared_ptr::noref(m_p);
            }
            
            constexpr operator T**() && noexcept
                { return &m_p; }
        private:
            constexpr output_param(intrusive_shared_ptr<T, Traits> & owner, T * p) noexcept :
                m_owner(&owner),
                m_p(p)
            {}
            constexpr output_param(output_param && src) noexcept = default;
            
            output_param(const output_param &) = delete;
            void operator=(const output_param &) = delete;
            void operator=(output_param &&) = delete;
        private:
            intrusive_shared_ptr<T, Traits> * m_owner;
            T * m_p;
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
        
        
        INTRUSIVE_SHARED_PTR_CONSTEXPR_SINCE_CPP20 ~intrusive_shared_ptr() noexcept
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
            { return output_param(*this, this->m_p); }
        
        constexpr T * release() noexcept
        { 
            T * p = this->m_p;
            this->m_p = nullptr;
            return p;
        }
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

    #if INTRUSIVE_SHARED_PTR_USE_SPACESHIP_OPERATOR

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



    template<class Dest, class Src, class Traits>
    inline constexpr
    std::enable_if_t<is_intrusive_shared_ptr_v<Dest>,
    Dest> intrusive_const_cast(intrusive_shared_ptr<Src, Traits> p) noexcept
    {
        return Dest::noref(const_cast<typename Dest::pointer>(p.release()));
    }

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
            exchange(desired, order);
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
        std::atomic<T *> m_p;
    };
}

#undef INTRUSIVE_SHARED_PTR_TRIVIAL_ABI
#undef INTRUSIVE_SHARED_PTR_CONSTEXPR_SINCE_CPP20
#undef INTRUSIVE_SHARED_PTR_USE_SPACESHIP_OPERATOR

#endif

