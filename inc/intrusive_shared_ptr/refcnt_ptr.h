/*
 Copyright 2004 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
*/

#ifndef HEADER_REFCNT_PTR_H_INCLUDED
#define HEADER_REFCNT_PTR_H_INCLUDED

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

namespace isptr
{

    ISPTR_EXPORTED
    template<class T>
    using refcnt_ptr = intrusive_shared_ptr<T, typename T::refcnt_ptr_traits>;

    ISPTR_EXPORTED
    template<class T>
    constexpr refcnt_ptr<T> refcnt_retain(T * ptr) noexcept {
        return refcnt_ptr<T>::ref(ptr);
    }

    ISPTR_EXPORTED
    template<class T>
    constexpr refcnt_ptr<T> refcnt_attach(T * ptr) noexcept {
        return refcnt_ptr<T>::noref(ptr);
    }

    ISPTR_EXPORTED
    template<class T, class... Args>
    inline refcnt_ptr<T> make_refcnt(Args &&... args) {
        return refcnt_ptr<T>::noref(new T( std::forward<Args>(args)... ));
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<typename T::weak_value_type> weak_cast(const refcnt_ptr<T> & src) {
        return src->get_weak_ptr();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<const typename T::weak_value_type> weak_cast(const refcnt_ptr<const T> & src) {
        return src->get_weak_ptr();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<typename T::strong_value_type> strong_cast(const refcnt_ptr<T> & src) noexcept {
        return src->lock();
    }

    ISPTR_EXPORTED
    template<class T>
    inline
    refcnt_ptr<const typename T::strong_value_type> strong_cast(const refcnt_ptr<const T> & src) noexcept {
        return src->lock();
    }
}

#endif
