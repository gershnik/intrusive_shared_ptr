#if !ISPTR_USE_MODULES
    #include <intrusive_shared_ptr/apple_cf_ptr.h>
#endif

#include <doctest/doctest.h>

#if ISPTR_USE_MODULES
    import isptr;
#endif

#if (defined(__APPLE__) && defined(__MACH__))

using namespace isptr;

TEST_SUITE("apple") {

TEST_CASE( "Apple Ptr" ) {
    
    auto str = cf_attach(CFStringCreateWithCString(nullptr, "Hello", kCFStringEncodingUTF8));
    CHECK( CFStringGetLength(str.get()) == 5 );
}

}

#endif


