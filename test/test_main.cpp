#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#if ISPTR_USE_PYTHON
    #include <Python.h>
#endif

int main(int argc, char** argv)
{
#if ISPTR_USE_PYTHON
    Py_Initialize();
#endif
    int ret = Catch::Session().run( argc, argv );
#if ISPTR_USE_PYTHON
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
#endif
    return ret;
}
