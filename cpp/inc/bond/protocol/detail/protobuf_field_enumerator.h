// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#include <bond/core/config.h>

#include <bond/core/runtime_schema.h>


namespace bond
{
namespace detail
{
namespace proto
{
    template <typename Schema>
    class FieldEnumerator;

    template <>
    class FieldEnumerator<RuntimeSchema>
    {
    public:
        explicit FieldEnumerator(const RuntimeSchema& schema)
            : _begin{ schema.GetStruct().fields.begin() },
              _end{ schema.GetStruct().fields.end() },
              _it{ _begin }
        {}

        const FieldDef* find(uint16_t id)
        {
            if (_it == _end
                || (_it->id != id && (++_it == _end || _it->id != id)))
            {
                _it = std::lower_bound(_begin, _end, id,
                    [](const FieldDef& f, uint16_t id) { return f.id < id; });
            }

            return _it != _end && _it->id == id ? &*_it : nullptr;
        }

    private:
        const std::vector<FieldDef>::const_iterator _begin;
        const std::vector<FieldDef>::const_iterator _end;
        std::vector<FieldDef>::const_iterator _it;
    };

    template <typename Schema>
    FieldEnumerator<Schema> EnumerateFields(const Schema& schema)
    {
        return FieldEnumerator<Schema>{ schema };
    }

} // namespace proto
} // namespace detail
} // namespace bond
