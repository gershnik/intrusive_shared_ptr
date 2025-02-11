#if ISPTR_USE_MODULES
    import isptr;
#else
    #include <intrusive_shared_ptr/intrusive_shared_ptr.h>
#endif

#include <doctest/doctest.h>
#include "mocks.h"

#include <atomic>
#include <type_traits>

using namespace isptr;

TEST_SUITE("traits") {

TEST_CASE( "Atomic type traits are correct" ) {

    using ptr = std::atomic<mock_ptr<instrumented_counted<1>>>;

    SUBCASE("Construction, destruction and assignment") {

        CHECK( sizeof(ptr) == sizeof(std::atomic<instrumented_counted<> *>) );
        CHECK( std::alignment_of_v<ptr> == std::alignment_of_v<std::atomic<instrumented_counted<> *>> );

        CHECK( std::is_default_constructible_v<ptr> );
        CHECK( std::is_nothrow_default_constructible_v<ptr> );
        CHECK( !std::is_trivially_default_constructible_v<ptr> );

        CHECK( !std::is_copy_constructible_v<ptr> );
        CHECK( !std::is_move_constructible_v<ptr> );
        
        CHECK( !std::is_copy_assignable_v<ptr> );
        CHECK( !std::is_move_assignable_v<ptr> );
        
        CHECK( !std::is_swappable_v<ptr> );
        
        CHECK( std::is_destructible_v<ptr> );
        CHECK( !std::is_trivially_destructible_v<ptr> );
        CHECK( std::is_nothrow_destructible_v<ptr> );

        CHECK( !std::is_assignable_v<ptr, ptr> );
        
        CHECK( std::is_constructible_v<ptr, mock_ptr<instrumented_counted<1>>> );
        CHECK( std::is_nothrow_constructible_v<ptr, mock_ptr<instrumented_counted<1>>> );
        CHECK( !std::is_trivially_constructible_v<ptr, mock_ptr<instrumented_counted<1>>> );

        CHECK( std::is_assignable_v<ptr, mock_ptr<instrumented_counted<1>>> );
        CHECK( std::is_nothrow_assignable_v<ptr, mock_ptr<instrumented_counted<1>>> );
        CHECK( !std::is_trivially_assignable_v<ptr, mock_ptr<instrumented_counted<1>>> );

        CHECK( std::is_convertible_v<ptr, mock_ptr<instrumented_counted<1>>> );
        //CHECK( std::is_nothrow_convertible_v<ptr, intrusive_shared_ptr<instrumented_counted<>>> );
    }
}

TEST_CASE( "Atomic load" ) {
    
    SUBCASE( "Explicit" ) {
        instrumented_counted<> object;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object);
        auto ptr1 = ptr.load();
        CHECK( ptr1.get() == &object );
        CHECK( object.count == 2 );
    }

    SUBCASE( "Implicit" ) {
        instrumented_counted<> object;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object);
        mock_ptr<instrumented_counted<1>> ptr1 = ptr;
        CHECK( ptr1.get() == &object );
        CHECK( object.count == 2 );
    }
    
    SUBCASE( "Free function" ) {
        instrumented_counted<> object;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object);
        mock_ptr<instrumented_counted<1>> ptr1 = std::atomic_load(&ptr);
        CHECK( ptr1.get() == &object );
        CHECK( object.count == 2 );
    }

}

TEST_CASE( "Atomic store" ) {
    
    SUBCASE( "Explicit" ) {
        instrumented_counted<> object1, object2;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object1);
        auto ptr1 = mock_noref(&object2);

        ptr.store(ptr1);
        CHECK( ptr1.get() == &object2 );
        CHECK( ptr.load().get() == &object2 );
        CHECK( object1.count == -1 );
        CHECK( object2.count == 2 );
    }

    SUBCASE( "Implicit" ) {
        instrumented_counted<> object1, object2;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object1);
        auto ptr1 = mock_noref(&object2);

        ptr = ptr1;
        CHECK( ptr1.get() == &object2 );
        CHECK( ptr.load().get() == &object2 );
        CHECK( object1.count == -1 );
        CHECK( object2.count == 2 );
    }
    
    SUBCASE( "Free function" ) {
        instrumented_counted<> object1, object2;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr = mock_noref(&object1);
        auto ptr1 = mock_noref(&object2);

        std::atomic_store(&ptr, ptr1);
        CHECK( ptr1.get() == &object2 );
        CHECK( ptr.load().get() == &object2 );
        CHECK( object1.count == -1 );
        CHECK( object2.count == 2 );
    }
    
}

TEST_CASE( "Atomic comapre and exchange" ) {

    SUBCASE( "Strong" ) {
        instrumented_counted<> object1, object2, object3;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr1 = mock_noref(&object1);
        auto ptr2 = mock_noref(&object2);
        auto ptr3 = mock_noref(&object3);

        auto res = ptr1.compare_exchange_strong(ptr2, ptr3);
        CHECK( !res );
        CHECK( ptr1.load().get() == &object1 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 2 );
        CHECK( object2.count == -1 );
        CHECK( object3.count == 1 );

        res = ptr1.compare_exchange_strong(ptr2, ptr3);
        CHECK( res );
        CHECK( ptr1.load().get() == &object3 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 1 );
        CHECK( object3.count == 2 );
    }

    SUBCASE( "Strong 2 arg" ) {
        instrumented_counted<> object1, object2, object3;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr1 = mock_noref(&object1);
        auto ptr2 = mock_noref(&object2);
        auto ptr3 = mock_noref(&object3);

        auto res = ptr1.compare_exchange_strong(ptr2, ptr3, std::memory_order_seq_cst, std::memory_order_seq_cst);
        CHECK( !res );
        CHECK( ptr1.load().get() == &object1 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 2 );
        CHECK( object2.count == -1 );
        CHECK( object3.count == 1 );

        res = ptr1.compare_exchange_strong(ptr2, ptr3, std::memory_order_seq_cst, std::memory_order_seq_cst);
        CHECK( res );
        CHECK( ptr1.load().get() == &object3 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 1 );
        CHECK( object3.count == 2 );
    }

    SUBCASE( "Weak" ) {
        instrumented_counted<> object1, object2, object3;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr1 = mock_noref(&object1);
        auto ptr2 = mock_noref(&object2);
        auto ptr3 = mock_noref(&object3);

        auto res = ptr1.compare_exchange_weak(ptr2, ptr3);
        CHECK( !res );
        CHECK( ptr1.load().get() == &object1 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 2 );
        CHECK( object2.count == -1 );
        CHECK( object3.count == 1 );

        res = ptr1.compare_exchange_weak(ptr2, ptr3);
        CHECK( res );
        CHECK( ptr1.load().get() == &object3 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 1 );
        CHECK( object3.count == 2 );
    }

    SUBCASE( "Weak 2 arg" ) {
        instrumented_counted<> object1, object2, object3;
        std::atomic<mock_ptr<instrumented_counted<>>> ptr1 = mock_noref(&object1);
        auto ptr2 = mock_noref(&object2);
        auto ptr3 = mock_noref(&object3);

        auto res = ptr1.compare_exchange_weak(ptr2, ptr3, std::memory_order_seq_cst, std::memory_order_seq_cst);
        CHECK( !res );
        CHECK( ptr1.load().get() == &object1 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 2 );
        CHECK( object2.count == -1 );
        CHECK( object3.count == 1 );

        res = ptr1.compare_exchange_weak(ptr2, ptr3, std::memory_order_seq_cst, std::memory_order_seq_cst);
        CHECK( res );
        CHECK( ptr1.load().get() == &object3 );
        CHECK( ptr2.get() == &object1 );
        CHECK( ptr3.get() == &object3 );
        CHECK( object1.count == 1 );
        CHECK( object3.count == 2 );
    }
}

}
