#if !ISPTR_USE_MODULES
    #include <intrusive_shared_ptr/ref_counted.h>
    #include <intrusive_shared_ptr/refcnt_ptr.h>
#endif

#include <doctest/doctest.h>

#include <vector>
#include <stdexcept>

#if ISPTR_USE_MODULES
    import isptr;
#endif

using namespace isptr;

namespace
{
    struct minimal_counted : ref_counted<minimal_counted, ref_counted_flags::none, char>
    {
        friend ref_counted;
        
    private:
        ~minimal_counted() noexcept
        {}
    };
    
    static_assert( !std::is_default_constructible_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_copy_constructible_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_move_constructible_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_copy_assignable_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_move_assignable_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_swappable_v<ref_counted<minimal_counted>> );
    static_assert( !std::is_destructible_v<ref_counted<minimal_counted>> );

    static_assert( sizeof(minimal_counted) == sizeof(char) );
    static_assert( !std::is_default_constructible_v<minimal_counted> );
    static_assert( !std::is_copy_constructible_v<minimal_counted> );
    static_assert( !std::is_move_constructible_v<minimal_counted> );
    static_assert( !std::is_copy_assignable_v<minimal_counted> );
    static_assert( !std::is_move_assignable_v<minimal_counted> );
    static_assert( !std::is_swappable_v<minimal_counted> );
    static_assert( !std::is_destructible_v<minimal_counted> );

    
    struct adapded { char c; };
    using minimal_adapded_counted = ref_counted_adapter<adapded, ref_counted_flags::none, char>;

    static_assert( sizeof(minimal_adapded_counted) == 2 * sizeof(char) );
    static_assert( !std::is_default_constructible_v<minimal_adapded_counted> );
    static_assert( !std::is_copy_constructible_v<minimal_adapded_counted> );
    static_assert( !std::is_move_constructible_v<minimal_adapded_counted> );
    static_assert( !std::is_copy_assignable_v<minimal_adapded_counted> );
    static_assert( !std::is_move_assignable_v<minimal_adapded_counted> );
    static_assert( !std::is_destructible_v<minimal_adapded_counted> );

    using minimal_wrapped_counted = ref_counted_wrapper<char, ref_counted_flags::none, char>;

    static_assert( sizeof(minimal_wrapped_counted) == 2 * sizeof(char) );
    static_assert( !std::is_default_constructible_v<minimal_wrapped_counted> );
    static_assert( !std::is_copy_constructible_v<minimal_wrapped_counted> );
    static_assert( !std::is_move_constructible_v<minimal_wrapped_counted> );
    static_assert( !std::is_copy_assignable_v<minimal_wrapped_counted> );
    static_assert( !std::is_move_assignable_v<minimal_wrapped_counted> );
    static_assert( !std::is_destructible_v<minimal_wrapped_counted> );
 

    struct simple_counted : ref_counted<simple_counted>
    {
        friend ref_counted;
        
        static inline int instance_count = 0;
        
        simple_counted() noexcept
        { ++instance_count; }
        
        simple_counted(int)
        { throw std::runtime_error("x"); }
    private:
        ~simple_counted() noexcept
        { --instance_count; }
    };

    static_assert( !std::is_default_constructible_v<simple_counted> );
    static_assert( !std::is_copy_constructible_v<simple_counted> );
    static_assert( !std::is_move_constructible_v<simple_counted> );
    static_assert( !std::is_copy_assignable_v<simple_counted> );
    static_assert( !std::is_move_assignable_v<simple_counted> );
    static_assert( !std::is_destructible_v<simple_counted> );

}

TEST_SUITE("ref_counted") {

TEST_CASE( "Minimal ref counted works" ) {

    SUBCASE("derived") {
        auto p1 = refcnt_attach(new minimal_counted());
        CHECK(p1);
        auto p2 = p1;
        CHECK(p2 == p1);
    }
    
    SUBCASE("adapted") {
        auto p1 = refcnt_attach(new minimal_adapded_counted(adapded{'a'}));
        CHECK(p1);
        CHECK(p1->c == 'a');
        auto p2 = p1;
        CHECK(p2 == p1);
    }
    
    SUBCASE("wrapped") {
        auto p1 = refcnt_attach(new minimal_wrapped_counted('a'));
        CHECK(p1);
        CHECK(p1->wrapped == 'a');
        auto p2 = p1;
        CHECK(p2 == p1);
    }
}

TEST_CASE( "Simple ref counted works" ) {
    
    auto p1 = refcnt_attach(new simple_counted());
    CHECK(simple_counted::instance_count == 1);
    auto p2 = p1;
    CHECK(simple_counted::instance_count == 1);
    p1.reset();
    CHECK(simple_counted::instance_count == 1);
    p2.reset();
    CHECK(simple_counted::instance_count == 0);
}

TEST_CASE( "Ref counted with ctor exception" ) {
    
    try
    {
        auto p1 = refcnt_attach(new simple_counted(2));
    }
    catch(std::exception &)
    {
        CHECK(simple_counted::instance_count == 0);
    }
}

TEST_CASE( "Custom destroy" ) {
    
    
    struct custom_destroy : ref_counted<custom_destroy>
    {
    friend ref_counted;
        
        custom_destroy(bool * d): destroyed(d) {}
        
        bool * destroyed;
    private:
        void destroy() const noexcept
        {
            *destroyed = true;
            free((void*)this);
        }
    };
    
    bool destroyed = false;
    auto p1 = refcnt_attach(new (malloc(sizeof(custom_destroy))) custom_destroy{&destroyed});
    p1.reset();
    CHECK(destroyed);
    
}

TEST_CASE( "Ref counted wrapper" ) {

    SUBCASE("adapter") {
        auto p1 = refcnt_attach(new ref_counted_adapter<std::vector<char>>(5));
        CHECK( p1->size() == 5 );
    }
    SUBCASE("wrapper") {
        auto p1 = refcnt_attach(new ref_counted_wrapper<std::vector<char>>(5));
        CHECK( p1->wrapped.size() == 5 );
    }
}

}
