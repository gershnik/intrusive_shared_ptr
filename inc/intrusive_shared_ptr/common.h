/*
 Copyright 2023 Eugene Gershnik

 Use of this source code is governed by a BSD-style
 license that can be found in the LICENSE file or at
 https://github.com/gershnik/intrusive_shared_ptr/blob/master/LICENSE.txt
*/

#ifndef HEADER_ISPTR_COMMON_H_INCLUDED
#define HEADER_ISPTR_COMMON_H_INCLUDED


#if __cpp_constexpr >= 201907L

    #define ISPTR_CONSTEXPR_SINCE_CPP20 constexpr

#else 

    #define ISPTR_CONSTEXPR_SINCE_CPP20

#endif

#if __cpp_impl_three_way_comparison >= 201907L

    #include <compare>

    #define ISPTR_USE_SPACESHIP_OPERATOR 1

#else 

    #define ISPTR_USE_SPACESHIP_OPERATOR 0

#endif


#ifdef _MSC_VER

    #define ISPTR_ALWAYS_INLINE __forceinline
    #define ISPTR_TRIVIAL_ABI

#elif defined(__clang__) 

    #define ISPTR_ALWAYS_INLINE [[gnu::always_inline]] inline
    #define ISPTR_TRIVIAL_ABI [[clang::trivial_abi]]

#elif defined (__GNUC__)

    #define ISPTR_ALWAYS_INLINE [[gnu::always_inline]] inline
    #define ISPTR_TRIVIAL_ABI

#endif

#ifndef ISPTR_EXPORTED
    #define ISPTR_EXPORTED
#endif


namespace isptr::internal
{
    template<bool Val, class... Args>
    constexpr bool dependent_bool = Val;
}


#endif

