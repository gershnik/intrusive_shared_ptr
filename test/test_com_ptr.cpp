
#if !ISPTR_USE_MODULES
    #include <intrusive_shared_ptr/com_ptr.h>
#endif

#include <doctest/doctest.h>

#if ISPTR_USE_MODULES
    #ifdef _WIN32
        #include <Unknwn.h>
    #endif
    import isptr;
#endif

#ifdef _WIN32

using namespace isptr;

TEST_SUITE("com") {

TEST_CASE( "COM Ptr") {

    com_shared_ptr<IStream> pStream;
#if ISPTR_SUPPORT_OUT_PTR
    HRESULT res = CreateStreamOnHGlobal(nullptr, true, std::out_ptr(pStream));
#else
    HRESULT res = CreateStreamOnHGlobal(nullptr, true, pStream.get_output_param());
#endif
    CHECK( SUCCEEDED(res) );
    CHECK( pStream );
}

}

#endif

