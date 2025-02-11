#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>

#if ISPTR_USE_PYTHON
    #include <Python.h>
#endif

int main(int argc, char** argv)
{
#if ISPTR_USE_PYTHON
    Py_Initialize();
#endif
    int ret = doctest::Context(argc, argv).run();
#if ISPTR_USE_PYTHON
    if (Py_FinalizeEx() < 0) {
        return 120;
    }
#endif
    return ret;
}
