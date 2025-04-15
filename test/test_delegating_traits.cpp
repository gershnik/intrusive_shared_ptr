#if !ISPTR_USE_MODULES
    #include <intrusive_shared_ptr/ref_counted.h>
    #include <intrusive_shared_ptr/refcnt_ptr.h>
#endif

#include <doctest/doctest.h>

#include <type_traits>
#include <cstdint>
#include <cstddef>

#if ISPTR_USE_MODULES
    import isptr;
#endif

using namespace isptr;


namespace
{
    template<class T, class Inner, class Converter>
    class ref_counted_delegating_traits;

    template<class T, class Inner, class Converter>
    struct ref_counted_weak_delegating_traits
    {
        using strong_value_type = Inner;
        using strong_ptr_traits = ref_counted_delegating_traits<T, Inner, Converter>;
        
        static void add_ref(const void * ref) noexcept
            { real_weak_from_delegating_weak(ref)->add_ref(); }
        
        static void sub_ref(const void * ref) noexcept
            { real_weak_from_delegating_weak(ref)->sub_ref(); }
        
        static const Inner * lock(const void * ref) noexcept
        {
            auto outer_strong = real_weak_from_delegating_weak(ref)->lock_owner();
            return outer_strong ? delegating_from_real(outer_strong) : nullptr;
        }
        
        static Inner * lock(void * ref) noexcept
        {
            auto outer_strong = real_weak_from_delegating_weak(ref)->lock_owner();
            return outer_strong ? delegating_from_real(outer_strong) : nullptr;
        }
    private:
        static T * real_from_delegating(Inner * pinner)
            { return const_cast<T *>(Converter::real_from_delegating(const_cast<const Inner *>(pinner))); }
        static const T * real_from_delegating(const Inner * pinner)
            { return const_cast<const T *>(Converter::real_from_delegating(pinner)); }
        
        static Inner * delegating_from_real(T * pouter)
            { return const_cast<Inner *>(Converter::delegating_from_real(const_cast<const T *>(pouter))); }
        static const Inner * delegating_from_real(const T * pouter)
            { return const_cast<const Inner *>(Converter::delegating_from_real(pouter)); }
        
        static typename T::weak_value_type * real_weak_from_delegating_weak(void * ref)
            { return static_cast<typename T::weak_value_type *>(ref); }
        static const typename T::weak_value_type * real_weak_from_delegating_weak(const void * ref)
            { return static_cast<const typename T::weak_value_type *>(ref); }
    };

    template<class T, class Inner, class Converter>
    class ref_counted_delegating_traits
    {
    public:
        using weak_value_type = void;
        
    public:
        using weak_ptr_traits = std::conditional_t<T::provides_weak_references, ref_counted_weak_delegating_traits<T, Inner, Converter>, void>;
        
        static void add_ref(const Inner * pinner) noexcept
            { real_from_delegating(pinner)->add_ref(); }
        
        static void sub_ref(const Inner * pinner) noexcept
            { real_from_delegating(pinner)->sub_ref(); }
        
        static const void * get_weak_value(const Inner * pinner)
            { return real_from_delegating(pinner)->get_weak_value(); }
        
        static void * get_weak_value(Inner * pinner)
            { return const_cast<void *>(ref_counted_delegating_traits::get_weak_value(const_cast<const Inner *>(pinner))); }
        
    private:
        static T * real_from_delegating(Inner * pinner)
            { return const_cast<T *>(Converter::real_from_delegating(const_cast<const Inner *>(pinner))); }
        static const T * real_from_delegating(const Inner * pinner)
            { return const_cast<const T *>(Converter::real_from_delegating(pinner)); }
    };

    template<class T, class Inner, class Converter>
    inline
    intrusive_shared_ptr<void, ref_counted_weak_delegating_traits<T,Inner, Converter>>
    weak_cast(const intrusive_shared_ptr<Inner, ref_counted_delegating_traits<T,Inner, Converter>> & src)
    {
        using dst_type = intrusive_shared_ptr<void, ref_counted_weak_delegating_traits<T,Inner, Converter>>;
        return dst_type::noref(ref_counted_delegating_traits<T,Inner, Converter>::get_weak_value(src.get()));
    }

    template<class T, class Inner, class Converter>
    inline
    intrusive_shared_ptr<const void, ref_counted_weak_delegating_traits<T,Inner, Converter>>
    weak_cast(const intrusive_shared_ptr<const Inner, ref_counted_delegating_traits<T,Inner, Converter>> & src)
    {
        using dst_type = intrusive_shared_ptr<const void, ref_counted_weak_delegating_traits<T,Inner, Converter>>;
        return dst_type::noref(ref_counted_delegating_traits<T,Inner, Converter>::get_weak_value(src.get()));
    }

    template<class T, class Inner, class Converter>
    inline
    intrusive_shared_ptr<Inner, ref_counted_delegating_traits<T,Inner, Converter>>
    strong_cast(const intrusive_shared_ptr<void, ref_counted_weak_delegating_traits<T,Inner, Converter>> & src) noexcept
    {
        using dst_type = intrusive_shared_ptr<Inner, ref_counted_delegating_traits<T,Inner, Converter>>;
        return dst_type::noref(ref_counted_weak_delegating_traits<T,Inner, Converter>::lock(src.get()));
    }

    template<class T, class Inner, class Converter>
    inline
    intrusive_shared_ptr<const Inner, ref_counted_delegating_traits<T,Inner, Converter>>
    strong_cast(const intrusive_shared_ptr<const void, ref_counted_weak_delegating_traits<T,Inner, Converter>> & src) noexcept
    {
        using dst_type = intrusive_shared_ptr<const Inner, ref_counted_delegating_traits<T,Inner, Converter>>;
        return dst_type::noref(ref_counted_weak_delegating_traits<T,Inner, Converter>::lock(src.get()));
    }

    class outer : public ref_counted<outer>
    {
    friend ref_counted;
    private:
        struct inner_converter
        {
            static const outer * real_from_delegating(const int * pinner) noexcept
            {
                outer * dummy = nullptr;
                size_t distance = (uintptr_t)&(dummy->_inner) - (uintptr_t)dummy;
                return (const outer *)((std::byte *)pinner - distance);
                //return offsetof(outer, _inner);
            }
        };
        
        using inner_traits = ref_counted_delegating_traits<outer, int, inner_converter>;
        friend ref_counted_delegating_traits<outer, int, inner_converter>;
    public:
        using inner_ptr = intrusive_shared_ptr<int, inner_traits>;
        using const_inner_ptr = intrusive_shared_ptr<const int, inner_traits>;
        
        inner_ptr get_inner_ptr() noexcept
        {
            return inner_ptr::ref(&_inner);
        }
        const_inner_ptr get_inner_ptr() const noexcept
        {
            return const_inner_ptr::ref(&_inner);
        }

        int & inner() 
            { return _inner; }
    private:
        int _inner = 0;
    };

    class weak_outer : public ref_counted<weak_outer, ref_counted_flags::provide_weak_references>
    {
        friend ref_counted;
    private:
        struct inner_converter
        {
            static const weak_outer * real_from_delegating(const int * pinner) noexcept
            {
                weak_outer * dummy = nullptr;
                size_t distance = (uintptr_t)&(dummy->_inner) - (uintptr_t)dummy;
                return (const weak_outer *)((std::byte *)pinner - distance);
                //return offsetof(outer, _inner);
            }
            static const int * delegating_from_real(const weak_outer * pouter) noexcept
                { return &pouter->_inner; }
        };
        
        using inner_traits = ref_counted_delegating_traits<weak_outer, int, inner_converter>;
        friend ref_counted_delegating_traits<weak_outer, int, inner_converter>;
        
        
    public:
        using inner_ptr = intrusive_shared_ptr<int, inner_traits>;
        using const_inner_ptr = intrusive_shared_ptr<const int, inner_traits>;
        
        using weak_inner_ptr = intrusive_shared_ptr<void, inner_traits::weak_ptr_traits>;
        using const_weak_inner_ptr = intrusive_shared_ptr<const void, inner_traits::weak_ptr_traits>;
        
        class weak_value_type : public weak_reference<weak_outer>
        {
        friend weak_outer;
        friend ref_counted_weak_delegating_traits<weak_outer, int, inner_converter>;
        private:
            weak_value_type(intptr_t count, weak_outer * owner): weak_reference(count, owner)
            {};
        };
        
        inner_ptr get_inner_ptr() noexcept
        {
            return inner_ptr::ref(&_inner);
        }
        const_inner_ptr get_inner_ptr() const noexcept
        {
            return const_inner_ptr::ref(&_inner);
        }
        
        weak_inner_ptr get_weak_inner_ptr()
        {
            return weak_inner_ptr::noref(inner_traits::get_weak_value(&_inner));
        }
        
        const_weak_inner_ptr get_weak_inner_ptr() const
        {
            return const_weak_inner_ptr::noref(inner_traits::get_weak_value(&_inner));
        }
        
        weak_value_type * make_weak_reference(intptr_t count) const
        {
            return new weak_value_type(count, const_cast<weak_outer *>(this));
        }

        int & inner()
            { return _inner; }
    private:
        int _inner = 0;
    };
}

TEST_SUITE("inner_ref_counted") {

TEST_CASE( "Inner counting" ) {
    
    auto pouter = refcnt_attach(new outer());
    auto pinner = pouter->get_inner_ptr();
    CHECK(pinner);
    *pinner = 3;
    CHECK(pouter->inner() == 3);
}

TEST_CASE( "Weak inner counting" ) {
    
    SUBCASE( "Non const" ) {
        auto pouter = refcnt_attach(new weak_outer());
        auto pinner1 = pouter->get_inner_ptr();
        CHECK(pinner1);
        auto weak1 = pouter->get_weak_inner_ptr();
        CHECK(weak1);
        auto weak2 = weak_cast(pinner1);
        CHECK(weak1 == weak2);
        
        auto pinner2 = strong_cast(weak2);
        CHECK(pinner2 == pinner1);
    }
    
    SUBCASE( "Const" ) {
        auto pouter = refcnt_attach(const_cast<const weak_outer *>(new weak_outer()));
        auto pinner1 = pouter->get_inner_ptr();
        CHECK(pinner1);
        auto weak1 = pouter->get_weak_inner_ptr();
        CHECK(weak1);
        auto weak2 = weak_cast(pinner1);
        CHECK(weak1 == weak2);
        
        auto pinner2 = strong_cast(weak2);
        CHECK(pinner2 == pinner1);
    }
}

}
