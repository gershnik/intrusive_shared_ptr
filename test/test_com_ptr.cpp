#include <intrusive_shared_ptr/com_ptr.h>

#include "catch.hpp"

#ifdef _WIN32

using namespace isptr;

TEST_CASE( "COM Ptr", "[com]") {

    com_shared_ptr<IStream> pStream;
    HRESULT res = CreateStreamOnHGlobal(nullptr, true, pStream.get_output_param());
    CHECK( SUCCEEDED(res) );
    CHECK( pStream );
}

#endif

