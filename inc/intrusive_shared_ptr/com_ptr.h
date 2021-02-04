/*
 Copyright 2004 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
*/

#ifndef HEADER_COM_PTR_H_INCLUDED
#define HEADER_COM_PTR_H_INCLUDED

#if defined(_WIN32)

#include <Unknwn.h>

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

namespace isptr
{
    struct com_traits
    {
        static void add_ref(IUnknown * ptr) noexcept
            { ptr->AddRef(); }
        static void sub_ref(IUnknown * ptr) noexcept
            { ptr->Release(); }
    };

    template<class T>
    using com_shared_ptr = intrusive_shared_ptr<T, com_traits>;

    template<class T>
    com_shared_ptr<T> com_retain(T * ptr) {
        return com_shared_ptr<T>::ref(ptr);
    }
    template<class T>
    com_shared_ptr<T> com_attach(T * ptr) {
        return com_shared_ptr<T>::noref(ptr);
    }
}

#endif

#endif
