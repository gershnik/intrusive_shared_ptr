#ifndef HEADER_REFCNT_PTR_H_INCLUDED
#define HEADER_REFCNT_PTR_H_INCLUDED

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

namespace isptr
{

    template<class T>
    using refcnt_ptr = intrusive_shared_ptr<T, typename T::refcnt_ptr_traits>;

    template<class T>
    refcnt_ptr<T> refcnt_retain(T * ptr) {
        return refcnt_ptr<T>::ref(ptr);
    }

    template<class T>
    refcnt_ptr<T> refcnt_attach(T * ptr) {
        return refcnt_ptr<T>::noref(ptr);
    }

    template<class T, class... Args>
    inline refcnt_ptr<T> make_refcnt(Args &&... args)
    {
        return refcnt_ptr<T>::noref(new T( std::forward<Args>(args)... ));
    }

    template<class T>
    inline
    refcnt_ptr<typename T::weak_value_type> weak_cast(const refcnt_ptr<T> & src)
    {
        return src->get_weak_ptr();
    }

    template<class T>
    inline
    refcnt_ptr<const typename T::weak_value_type> weak_cast(const refcnt_ptr<const T> & src)
    {
        return src->get_weak_ptr();
    }

    template<class T>
    inline
    refcnt_ptr<typename T::strong_value_type> strong_cast(const refcnt_ptr<T> & src) noexcept
    {
        return src->lock();
    }

    template<class T>
    inline
    refcnt_ptr<const typename T::strong_value_type> strong_cast(const refcnt_ptr<const T> & src) noexcept
    {
        return src->lock();
    }
}

#endif
