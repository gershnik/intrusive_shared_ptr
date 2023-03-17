
#if ISPTR_USE_PYTHON

#define PY_SSIZE_T_CLEAN
#include <intrusive_shared_ptr/python_ptr.h>

#include "catch.hpp"

using namespace isptr;

TEST_CASE( "Python Ptr", "[python]") {
    
    auto str = py_attach(PyUnicode_FromString("Hello"));
    REQUIRE(str);
    CHECK( PyUnicode_GetLength(str.get()) == 5 );
}

#endif


