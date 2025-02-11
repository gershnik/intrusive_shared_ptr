
#if ISPTR_USE_PYTHON

#if ISPTR_USE_MODULES
    import isptr;

    #define PY_SSIZE_T_CLEAN
    #include <Python.h>
#else
    #define PY_SSIZE_T_CLEAN
    #include <intrusive_shared_ptr/python_ptr.h>
#endif

#include <doctest/doctest.h>

using namespace isptr;

TEST_SUITE("python") {

TEST_CASE( "Python Ptr" ) {
    
    auto str = py_attach(PyUnicode_FromString("Hello"));
    REQUIRE(str);
    CHECK( PyUnicode_GetLength(str.get()) == 5 );
}

}

#endif


