
#pragma once

#include "service_types.h"
#include <bond/core/reflection.h>
#include "basic_types_reflection.h"
#include "namespace_basic_types_reflection.h"

namespace tests
{
    //
    // dummy
    //
    struct dummy::Schema
    {
        typedef ::bond::no_base base;

        DllExport
        static const ::bond::Metadata metadata;
        
        private: DllExport
        static const ::bond::Metadata s_count_metadata;

        public: struct var
        {
            // count
            typedef struct : ::bond::reflection::FieldTemplate<
                0,
                0,
                ::bond::reflection::optional_field_modifier,
                dummy,
                int32_t,
                &dummy::count,
                &s_count_metadata
            > {} count;
        };

        using field_count = std::integral_constant<uint16_t, 1>;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4348) // VC bug: redefinition of default parameter
#endif
        template <uint16_t I, int = 0> struct field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
        template <int __bond_dummy> struct field<0, __bond_dummy> : ::bond::detail::mpl::identity<var::count> {};
        
        
        static ::bond::Metadata GetMetadata()
        {
            return ::bond::reflection::MetadataInit("dummy", "tests.dummy",
                ::bond::reflection::Attributes()
            );
        }
    };
    

    
} // namespace tests
