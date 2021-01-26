#include <intrusive_shared_ptr/apple_cf_ptr.h>

#include "catch.hpp"

#if (defined(__APPLE__) && defined(__MACH__))

using namespace isptr;

TEST_CASE( "Apple Ptr", "[apple]") {
    
    auto str = cf_attach(CFStringCreateWithCString(nullptr, "Hello", kCFStringEncodingUTF8));
    CHECK( CFStringGetLength(str.get()) == 5 );
}

#endif


