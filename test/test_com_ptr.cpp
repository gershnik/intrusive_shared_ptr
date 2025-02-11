
#if ISPTR_USE_MODULES
    import isptr;
    #ifdef _WIN32
        #include <Unknwn.h>
    #endif
#else
    #include <intrusive_shared_ptr/com_ptr.h>
#endif

#include <doctest/doctest.h>

#ifdef _WIN32

using namespace isptr;

TEST_SUITE("com") {

TEST_CASE( "COM Ptr") {

    com_shared_ptr<IStream> pStream;
    HRESULT res = CreateStreamOnHGlobal(nullptr, true, pStream.get_output_param());
    CHECK( SUCCEEDED(res) );
    CHECK( pStream );
}

}

#endif

