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
    struct RequiredFieldInfo
    {
        explicit RequiredFieldInfo(uint16_t id)
            : id{ id },
              visited{ false }
        {}

        uint16_t id;
        bool visited;
    };


    template <typename T>
    class RequiredFieldIds
    {
    public:
        using type = std::array<
            RequiredFieldInfo,
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
                *it++ = RequiredFieldInfo{ Field::id };
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


    template <typename Iterator>
    inline Iterator FindNextField(Iterator begin, Iterator end, Iterator hint, uint16_t id)
    {
        if (hint == end || (hint->id != id && (++hint == end || hint->id != id)))
        {
            hint = std::lower_bound(
                begin,
                end,
                id,
                [](typename std::iterator_traits<Iterator>::reference item, uint16_t id)
                {
                    return item.id < id;
                });
        }

        return hint != end && hint->id == id ? hint : end;
    }


    template <typename T>
    class RequiredFieldUnorderedValiadator
    {
        using count = boost::mpl::count_if<typename schema<T>::type::fields, is_required_field<_> >;

    public:
        BOOST_STATIC_ASSERT(!no_required_fields<T>::value);
        BOOST_STATIC_ASSERT(count::value != 0);

        void Begin(const T& /*var*/)
        {
            _ids = RequiredFieldIds<T>::instance;
            _it = _ids.begin();
            _count = 0;
        }

        template <typename Head>
        void Validate()
        {
            BOOST_STATIC_ASSERT(std::is_same<typename Head::field_modifier,
                                            reflection::required_field_modifier>::value);

            _it = FindNextField(_ids.begin(), _ids.end(), _it, Head::id);

            BOOST_ASSERT(_it != _ids.end());
            BOOST_ASSERT(_it->id == Head::id);

            if (!_it->second)
            {
                _it->second = true;
                ++_count;
            }
        }

        void End() const
        {
            if (_count != count::value)
            {
                BOND_THROW(CoreException,
                    "De-serialization failed: one or more required fields are missing from "
                    << schema<T>::type::metadata.qualified_name);
            }
        }

        bool operator==(const RequiredFieldUnorderedValiadator&) const
        {
            BOOST_ASSERT(false);
            return false;
        }

    private:
        typename RequiredFieldIds<T>::type::iterator _it;
        uint16_t _count;
        typename RequiredFieldIds<T>::type _ids;
    };


    template <typename T>
    using always_one = std::integral_constant<uint16_t, 1>;

    template <typename T>
    class RequiredFieldValiadator
    {
    protected:
        template <typename U>
        struct rebind
        {
            using type = RequiredFieldValiadator<U>;
        };

        RequiredFieldValiadator()
        {
            Init();
        }

        template <typename U = T>
        typename boost::enable_if<no_required_fields<U> >::type
        Begin(const T& /*var*/) const
        {}

        template <typename U = T>
        typename boost::disable_if<no_required_fields<U> >::type
        Begin(const T& var) const
        {
            _impl.template unsafe_cast<RequiredFieldUnorderedValiadator<T> >().Begin(var);
        }

        template <typename Head>
        typename boost::enable_if<std::is_same<typename Head::field_modifier,
                                                reflection::required_field_modifier> >::type
        Validate() const
        {
            _impl.template unsafe_cast<RequiredFieldUnorderedValiadator<T> >().template Validate<Head>();
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
            _impl.template unsafe_cast<RequiredFieldUnorderedValiadator<T> >().End();
        }

    private:
        template <typename U = T>
        typename boost::enable_if<no_required_fields<U> >::type
        Init()
        {}

        template <typename U = T>
        typename boost::disable_if<no_required_fields<U> >::type
        Init()
        {
            _impl = RequiredFieldUnorderedValiadator<T>{};
        }

        mutable any<always_one, 64> _impl;
    };

} // namespace proto
} // namespace detail
} // namespace bond
