#ifndef HEADER_APPLE_CF_PTR_H_INCLUDED
#define HEADER_APPLE_CF_PTR_H_INCLUDED

#if (defined(__APPLE__) && defined(__MACH__))

#include <CoreFoundation/CoreFoundation.h>

#include <intrusive_shared_ptr/intrusive_shared_ptr.h>

namespace isptr
{
    struct cf_traits
    {
        static void add_ref(CFTypeRef ptr) noexcept
            { CFRetain(ptr); }
        static void sub_ref(CFTypeRef ptr) noexcept
            { CFRelease(ptr); }
    };

    template<class T>
    using cf_ptr = intrusive_shared_ptr<std::remove_pointer_t<T>, cf_traits>;

    template<class T>
    cf_ptr<T *> cf_retain(T * ptr) {
        return cf_ptr<T *>::ref(ptr);
    }
    template<class T>
    cf_ptr<T *> cf_attach(T * ptr) {
        return cf_ptr<T *>::noref(ptr);
    }
}

#endif

#endif
