

template<int Tag = 1>
struct instrumented_counted
{
    mutable int count = 1;

    virtual ~instrumented_counted()
    {
        CHECK(this->count == -1); 
    }
};

template<int Tag = 1>
struct derived_instrumented_counted : instrumented_counted<Tag>
{};

struct non_counted 
{};


template<int TraitsTag = 1>
struct mock_traits
{
    template<int Tag>
    static void add_ref(const instrumented_counted<Tag> * c) noexcept
    {
        REQUIRE(c->count > 0); 
        ++c->count; 
    }

    template<int Tag>
    static void sub_ref(const instrumented_counted<Tag> * c) noexcept
    { 
        CHECK(c->count > 0);
        if (--c->count == 0)
            c->count = -1;
    }
};

template<class T>
using mock_ptr = isptr::intrusive_shared_ptr<T, mock_traits<>>;

template<class T>
using mock_ptr_different_traits = isptr::intrusive_shared_ptr<T, mock_traits<2>>;

template<class T> 
mock_ptr<T> mock_ref(T * ptr) {
    return mock_ptr<T>::ref(ptr);
}
template<class T> 
mock_ptr<T> mock_noref(T * ptr) {
    return mock_ptr<T>::noref(ptr);
}

template<class T> 
mock_ptr_different_traits<T> mock_ref_different_traits(T * ptr) {
    return mock_ptr_different_traits<T>::ref(ptr);
}
template<class T> 
mock_ptr_different_traits<T> mock_noref_different_traits(T * ptr) {
    return mock_ptr_different_traits<T>::noref(ptr);
}
