#pragma once

#include <bond/core/bond.h>
#include <bond/protocol/detail/protobuf_utils.h>


namespace bond
{
namespace detail
{
namespace proto
{
    template <typename T>
    class AppendTo : public ModifyingTransform
    {
    public:
        using FastPathType = T;

        explicit AppendTo(const T& var)
            : _var{ var }
        {}

        void Begin(const Metadata&) const
        {}

        void End() const
        {}

        void UnknownEnd() const
        {}

        template <typename X>
        bool Base(const X&) const
        {
            BOOST_ASSERT(false);
        }

        template <typename FieldT, typename X>
        bool Field(const FieldT&, X& value) const
        {
            auto&& var = FieldT::GetVariable(_var);

            if (!is_default(var, FieldT::metadata))
            {
                Append(var, value);
            }

            return false;
        }

    private:
        template <typename X>
        typename boost::enable_if_c<is_basic_type<X>::value || is_blob_type<X>::value>::type
        Append(const X& var, X& value) const
        {
            value = var;
        }

        template <typename X>
        typename boost::enable_if<has_schema<X> >::type
        Append(const X& var, X& value) const
        {
            bond::Apply(AppendTo<X>{ var }, value);
        }

        template <typename X>
        typename boost::enable_if_c<(is_list_container<X>::value || is_set_container<X>::value)
                                    && !is_blob_type<X>::value>::type
        Append(const X& var, X& value) const
        {
            for (const_enumerator<X> items(var); items.more(); )
            {
                container_append(value, items.next());
            }
        }

        template <typename X>
        typename boost::enable_if<is_map_container<X> >::type
        Append(const X& var, X& value) const
        {
            for (const_enumerator<X> items(var); items.more(); )
            {
                const auto& item = items.next();
                Append(item.second, mapped_at(value, item.first));
            }
        }

        template <typename X>
        void Append(const nullable<X>& var, nullable<X>& value) const
        {
            if (var.hasvalue())
            {
                Append(var.value(), value.set());
            }
        }

        const T& _var;
    };

} // namespace proto
} // namespace detail
} // namespace bond
