// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include <bond/core/reflection.h>
#include <bond/core/detail/any.h>

#include <boost/mpl/count_if.hpp>
#include <boost/mpl/for_each.hpp>

#include <array>


namespace bond
{
namespace detail
{
namespace proto
{
    template <typename T>
    class RequiredFieldIds
    {
    public:
        using type = std::array<
            std::pair<uint16_t, bool>,
            boost::mpl::count_if<typename schema<T>::type::fields, is_required_field<_> >::value>;

        static const type& instance;

        RequiredFieldIds() = delete;

    private:
        struct Initializer
        {
            explicit Initializer(typename type::iterator begin)
                : it{ begin }
            {
                boost::mpl::for_each<typename schema<T>::type::fields>(boost::ref(*this));
            }

            template <typename Field>
            typename boost::enable_if<is_required_field<Field> >::type
            operator()(const Field&)
            {
                *it++ = std::make_pair(Field::id, false);
            }

            template <typename Field>
            typename boost::disable_if<is_required_field<Field> >::type
            operator()(const Field&)
            {}

            typename type::iterator it;
        };

        static const type& Init()
        {
            static type ids;
            Initializer init{ ids.begin() };
            BOOST_ASSERT(init.it == ids.end());
            return ids;
        }
    };

    template <typename T>
    const typename RequiredFieldIds<T>::type& RequiredFieldIds<T>::instance = Init();


    template <typename T>
    class RequiredFieldUnorderedValiadator
    {
    protected:
        template <typename U>
        struct rebind
        {
            using type = RequiredFieldUnorderedValiadator<U>;
        };

        template <typename U = T>
        typename boost::enable_if<no_required_fields<U> >::type
        Begin(const T& /*var*/) const
        {}

        template <typename U = T>
        typename boost::disable_if<no_required_fields<U> >::type
        Begin(const T& /*var*/) const
        {
            _ids = RequiredFieldIds<T>::instance;
            _it = GetIds().data();
            _count = 0;
        }

        template <typename Head>
        typename boost::enable_if<std::is_same<typename Head::field_modifier,
                                                reflection::required_field_modifier> >::type
        Validate() const
        {
            NextField(Head::id);

            if (!_it->second)
            {
                _it->second = true;
                ++_count;
            }
        }

        template <typename Head>
        typename boost::disable_if<std::is_same<typename Head::field_modifier,
                                                reflection::required_field_modifier> >::type
        Validate() const
        {}

        template <typename U = T>
        typename boost::enable_if<no_required_fields<U> >::type
        End() const
        {}

        template <typename U = T>
        typename boost::disable_if<no_required_fields<U> >::type
        End() const
        {
            using Count = boost::mpl::count_if<typename schema<T>::type::fields, is_required_field<_> >;

            if (_count != Count::value)
            {
                BOND_THROW(CoreException,
                    "De-serialization failed: one or more required fields are missing from "
                    << schema<T>::type::metadata.qualified_name);
            }
        }

    private:
        template <typename T>
        using always_one = std::integral_constant<uint16_t, 1>;

        using storage_size = std::integral_constant<std::size_t, sizeof(std::array<std::pair<uint16_t, bool>, 8>)>;

        template <typename U = T>
        typename RequiredFieldIds<U>::type& GetIds() const
        {
            BOOST_STATIC_ASSERT(std::is_same<T, U>::value);
            auto ids = _ids.template cast<typename RequiredFieldIds<T>::type>();
            BOOST_ASSERT(ids);
            return *ids;
        }

        void NextField(uint16_t id) const
        {
            using Count = boost::mpl::count_if<typename schema<T>::type::fields, is_required_field<_> >;

            auto& ids = GetIds();
            const auto begin = ids.data();
            const auto end = ids.data() + Count::value;

            if (_it == end
                || (_it->first != id && (++_it == end || _it->first != id)))
            {
                _it = std::lower_bound(begin, end, id,
                    [](const std::pair<uint16_t, bool>& pair, uint16_t id) { return pair.first < id; });
            }

            BOOST_ASSERT(_it != end);
            BOOST_ASSERT(_it->first == id);
        }

        mutable any<always_one, storage_size::value> _ids;
        mutable std::pair<uint16_t, bool>* _it;
        mutable uint16_t _count;
    };

} // namespace proto
} // namespace detail
} // namespace bond
