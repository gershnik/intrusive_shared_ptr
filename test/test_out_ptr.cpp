#if ISPTR_USE_MODULES
    import isptr;
#else
    #include <intrusive_shared_ptr/intrusive_shared_ptr.h>
#endif

#include <doctest/doctest.h>
#include "mocks.h"

using namespace isptr;

#if ISPTR_SUPPORT_OUT_PTR

TEST_SUITE("out_ptr") {

TEST_CASE( "out_ptr basics" ) {

    instrumented_counted<> items[2] = {};
    auto c_func = [&](size_t idx, instrumented_counted<> ** res) {
        *res = &items[idx];
    };

    mock_ptr<instrumented_counted<>> p;

    c_func(0, std::out_ptr(p));
    CHECK(p.get() == &items[0]);
    CHECK(items[0].count == 1);

    c_func(1, std::out_ptr(p));
    CHECK(p.get() == &items[1]);
    CHECK(items[0].count == -1);
    CHECK(items[1].count == 1);
}

TEST_CASE( "out_ptr void_pp" ) {

    instrumented_counted<> items[2] = {};
    auto c_func = [&](size_t idx, void ** res) {
        *res = &items[idx];
    };

    mock_ptr<instrumented_counted<>> p;

    c_func(0, std::out_ptr(p));
    CHECK(p.get() == &items[0]);
    CHECK(items[0].count == 1);

    c_func(1, std::out_ptr(p));
    CHECK(p.get() == &items[1]);
    CHECK(items[0].count == -1);
    CHECK(items[1].count == 1);
}

TEST_CASE( "inout_ptr basics" ) {

    instrumented_counted<> items[2] = {};
    auto c_func = [&](size_t idx, instrumented_counted<> ** res) {
        if (*res)
            mock_traits<>::sub_ref(*res);
        *res = &items[idx];
    };

    mock_ptr<instrumented_counted<>> p;

    c_func(0, std::inout_ptr(p));
    CHECK(p.get() == &items[0]);
    CHECK(items[0].count == 1);

    c_func(1, std::inout_ptr(p));
    CHECK(p.get() == &items[1]);
    CHECK(items[0].count == -1);
    CHECK(items[1].count == 1);
}

TEST_CASE( "inout_ptr void_pp" ) {

    instrumented_counted<> items[2] = {};
    auto c_func = [&](size_t idx, void ** res) {
        auto ptr = (instrumented_counted<> **)(res);
        if (*ptr)
            mock_traits<>::sub_ref(*ptr);
        *ptr = &items[idx];
    };

    mock_ptr<instrumented_counted<>> p;

    c_func(0, std::inout_ptr(p));
    CHECK(p.get() == &items[0]);
    CHECK(items[0].count == 1);

    c_func(1, std::inout_ptr(p));
    CHECK(p.get() == &items[1]);
    CHECK(items[0].count == -1);
    CHECK(items[1].count == 1);
}

}

#endif
