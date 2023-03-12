/*
 Copyright 2023 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
*/

#ifndef HEADER_PYTHON_PTR_H_INCLUDED
#define HEADER_PYTHON_PTR_H_INCLUDED

#include <Python.h>

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

namespace isptr
{
    struct py_traits {
        static void add_ref(PyObject * ptr) noexcept
            { Py_INCREF(ptr); }
        static void sub_ref(PyObject * ptr) noexcept
            { Py_DECREF(ptr); }

        static void add_ref(PyTypeObject * ptr) noexcept
            { Py_INCREF(ptr); }
        static void sub_ref(PyTypeObject * ptr) noexcept
            { Py_DECREF(ptr); }
    };

    template<class T>
    using py_ptr = intrusive_shared_ptr<T, py_traits>;

    template<class T>
    py_ptr<T> py_retain(T * ptr) {
        return py_ptr<T>::ref(ptr);
    }
    template<class T>
    py_ptr<T> py_attach(T * ptr) {
        return py_ptr<T>::noref(ptr);
    }
}

