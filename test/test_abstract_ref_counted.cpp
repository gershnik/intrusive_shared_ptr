#include <intrusive_shared_ptr/ref_counted.h>
#include <intrusive_shared_ptr/refcnt_ptr.h>

#include "catch.hpp"

using namespace isptr;


namespace
{
    class full_ref_counted : public ref_counted<full_ref_counted, ref_counted_flags::provide_weak_references | ref_counted_flags::use_abstract_base>
    {
    public:
        void add_ref() const noexcept final
        {
            ref_counted::add_ref();
        }
        
        void sub_ref() const noexcept final
        {
            ref_counted::sub_ref();
        }
    protected:
        ~full_ref_counted() noexcept
        {}
        
        ref_counted::weak_value_type * make_weak_reference(intptr_t count) const final;
    };

    class full_weak_reference : public weak_reference<full_ref_counted>
    {
    friend full_ref_counted;
    protected:
        ~full_weak_reference() noexcept
        {}
        
    private:
        full_weak_reference(intptr_t count, full_ref_counted * owner):
            weak_reference(count, owner)
        {}
        
        void add_owner_ref() noexcept final
        {
            weak_reference::add_owner_ref();
        }
        
        void sub_owner_ref() noexcept final
        {
            weak_reference::sub_owner_ref();
        }
        
        full_ref_counted * lock_owner() const noexcept final
        {
            return static_cast<full_ref_counted *>(weak_reference::lock_owner());
        }
        
        void on_owner_destruction() const noexcept final
        {
            
        }
    };

    inline auto full_ref_counted::make_weak_reference(intptr_t count) const -> ref_counted::weak_value_type *
    {
        return new full_weak_reference(count, const_cast<full_ref_counted *>(this));
    }

    class simple : public full_ref_counted
    {
        
    };
}


TEST_CASE( "Abstract ref counted works", "[abstract_ref_counted]") {
    
    SECTION( "Simple" ) {
        auto p = refcnt_attach(new simple());
        auto w = weak_cast(p);
        auto w1 = p->get_weak_ptr();
    }
}

