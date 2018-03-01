#include "precompiled.h"
#include "protobuf_reader_test_utils.h"
#include "protobuf_writer_reflection.h"

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(ProtobufReaderWireTypeMatchTests)

enum class WireTypeMatchFlag : uint8_t
{
    None = 0,
    VarInt = 0x1,
    Fixed32 = 0x2,
    Fixed64 = 0x4,
    LengthDelimited = 0x8,
    All = 0xF
};

inline WireTypeMatchFlag operator|(WireTypeMatchFlag left, WireTypeMatchFlag right)
{
    return static_cast<WireTypeMatchFlag>(uint8_t(left) | uint8_t(right));
}

inline bool operator&(WireTypeMatchFlag left, WireTypeMatchFlag right)
{
    return static_cast<WireTypeMatchFlag>(uint8_t(left) & uint8_t(right)) != WireTypeMatchFlag::None;
}

struct ScalarValues
{
    ScalarValues()
    {
        SetValue(varint_value);
        SetValue(fixed32_value);
        SetValue(fixed64_value);
        SetValue(lengthdelim_value);
    }

    google::protobuf::UInt32Value varint_value;
    google::protobuf::FloatValue fixed32_value;
    google::protobuf::DoubleValue fixed64_value;
    google::protobuf::BytesValue lengthdelim_value;
};

template <typename T>
class AllDefault
{
public:
    explicit AllDefault(const T& var)
        : _var{ var },
          _default{ true }
    {
        boost::mpl::for_each<typename bond::schema<T>::type::fields>(boost::ref(*this));
    }

    operator bool() const
    {
        return _default;
    }

    template <typename Field>
    void operator()(const Field&)
    {
        CheckDefault(Field::GetVariable(_var));
    }

private:
    template <typename X>
    typename boost::disable_if<bond::is_map_container<X> >::type
    CheckDefault(const X& value)
    {
        if (_default && value != X{})
        {
            _default = false;
        }
    }

    template <typename X>
    typename boost::enable_if<bond::is_map_container<X> >::type
    CheckDefault(const X& value)
    {
        if (_default && !value.empty())
        {
            if (value.size() == 1)
            {
                auto it = value.find(typename X::key_type{});
                if (it == value.end() || it->second != typename X::mapped_type{})
                {
                    _default = false;
                }
            }
            else
            {
                _default = false;
            }
        }
    }

    const T& _var;
    bool _default;
};

template <typename Bond, typename Proto>
void CheckFieldMatch(const Proto& proto_struct, bool match, bool strict)
{
    auto str = proto_struct.SerializeAsString();
    bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));

    // Compile-time schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };
        auto bond_struct = bond::Deserialize<Bond>(reader);
        BOOST_CHECK(match != AllDefault<Bond>{ bond_struct });
    }
    // Runtime schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };
        auto bond_struct = bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>());
        BOOST_CHECK(match != AllDefault<Bond>{ bond_struct });
    }
}

template <typename Proto, typename Bond>
void CheckFieldMatch(bool match, bool strict = true)
{
    auto bond_struct = InitRandom<Bond>();

    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    CheckFieldMatch<Bond>(proto_struct, match, strict);
}

template <typename T, typename U = ScalarValues>
void CheckWireTypeMatch(bond::detail::proto::WireType type, WireTypeMatchFlag mask, const U& values = {})
{
    using bond::detail::proto::WireType;

    BOOST_STATIC_ASSERT(boost::mpl::size<typename bond::schema<T>::type::fields>::value == 1);

    // strict match
    CheckFieldMatch<T>(values.varint_value, type == WireType::VarInt, true);
    CheckFieldMatch<T>(values.fixed32_value, type == WireType::Fixed32, true);
    CheckFieldMatch<T>(values.fixed64_value, type == WireType::Fixed64, true);
    CheckFieldMatch<T>(values.lengthdelim_value, type == WireType::LengthDelimited, true);

    // relaxed match
    CheckFieldMatch<T>(values.varint_value, mask & WireTypeMatchFlag::VarInt, false);
    CheckFieldMatch<T>(values.fixed32_value, mask & WireTypeMatchFlag::Fixed32, false);
    CheckFieldMatch<T>(values.fixed64_value, mask & WireTypeMatchFlag::Fixed64, false);
    CheckFieldMatch<T>(values.lengthdelim_value, mask & WireTypeMatchFlag::LengthDelimited, false);
}

template <typename T>
typename boost::disable_if<bond::is_container<typename T::Schema::var::value::field_type> >::type
CheckWireTypeMatch(WireTypeMatchFlag mask)
{
    using type_id = bond::get_type_id<typename T::Schema::var::value::field_type>;

    CheckWireTypeMatch<T>(
        bond::detail::proto::GetWireType(
            type_id::value,
            bond::detail::proto::ReadEncoding(type_id::value, &T::Schema::var::value::metadata)),
        mask);
}

template <typename T>
typename boost::enable_if_c<bond::is_list_container<typename T::Schema::var::value::field_type>::value
                            || bond::is_set_container<typename T::Schema::var::value::field_type>::value>::type
CheckWireTypeMatch(WireTypeMatchFlag mask)
{
    using bond::detail::proto::Packing;

    using type_id = bond::get_type_id<
        typename bond::element_type<typename T::Schema::var::value::field_type>::type>;

    Packing packing = bond::detail::proto::ReadPacking(type_id::value, &T::Schema::var::value::metadata);

    CheckWireTypeMatch<T>(
        packing == Packing::False
            ? bond::detail::proto::GetWireType(
                type_id::value,
                bond::detail::proto::ReadEncoding(type_id::value, &T::Schema::var::value::metadata))
            : bond::detail::proto::WireType::LengthDelimited,
        mask);
}

template <typename T>
typename boost::enable_if<bond::is_map_container<typename T::Schema::var::value::field_type> >::type
CheckMapKeyWireTypeMatch(WireTypeMatchFlag mask)
{
    struct Values
    {
        Values()
        {
            varint_value.add_value()->set_key(1);
            fixed32_value.add_value()->set_key(1);
            fixed64_value.add_value()->set_key(1);
            lengthdelim_value.add_value()->set_key(GetMagicBytes());
        }

        unittest::proto::MapKeyVarInt varint_value;
        unittest::proto::MapKeyFixed32 fixed32_value;
        unittest::proto::MapKeyFixed64 fixed64_value;
        unittest::proto::MapKeyLengthDelim lengthdelim_value;
    };

    using type_id = bond::get_type_id<typename T::Schema::var::value::field_type::key_type>;

    CheckWireTypeMatch<T, Values>(
        bond::detail::proto::GetWireType(
            type_id::value,
            bond::detail::proto::ReadKeyEncoding(type_id::value, &T::Schema::var::value::metadata)),
        mask);
}

template <typename T>
typename boost::enable_if<bond::is_map_container<typename T::Schema::var::value::field_type> >::type
CheckMapValueWireTypeMatch(WireTypeMatchFlag mask)
{
    struct Values
    {
        Values()
        {
            varint_value.add_value()->set_value(1);
            fixed32_value.add_value()->set_value(1);
            fixed64_value.add_value()->set_value(1);
            lengthdelim_value.add_value()->set_value(GetMagicBytes());
        }

        unittest::proto::MapValueVarInt varint_value;
        unittest::proto::MapValueFixed32 fixed32_value;
        unittest::proto::MapValueFixed64 fixed64_value;
        unittest::proto::MapValueLengthDelim lengthdelim_value;
    };

    using type_id = bond::get_type_id<typename T::Schema::var::value::field_type::mapped_type>;
    using is_blob = bond::detail::proto::is_blob_type<typename T::Schema::var::value::field_type::mapped_type>;

    CheckWireTypeMatch<T, Values>(
        is_blob::value
            ? bond::detail::proto::WireType::LengthDelimited
            : bond::detail::proto::GetWireType(
                type_id::value,
                bond::detail::proto::ReadValueEncoding(type_id::value, &T::Schema::var::value::metadata)),
        mask);
}

BOOST_AUTO_TEST_CASE(BoolWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<bool> > >(WireTypeMatchFlag::VarInt);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

using SignedIntegerWireTypeMatchTests_Types = expand<signed_integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerWireTypeMatchTests, T, SignedIntegerWireTypeMatchTests_Types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckWireTypeMatch<unittest::BoxZigZag<bond::maybe<T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(FloatPointIntegerWireTypeMatchTests, T, float_point_types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<T> > >(
        WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobWireTypeMatchTests, T, blob_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<T> > >(
        bond::detail::proto::WireType::LengthDelimited, WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(UnpackedIntegerContainerTests)
{
    CheckFieldMatch<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersContainer>(false);
    CheckFieldMatch<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersContainer>(false);
}

BOOST_AUTO_TEST_CASE(UnpackedIntegerSetContainerTests)
{
    CheckFieldMatch<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersSetContainer>(false);
    CheckFieldMatch<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersSetContainer>(false);
}

BOOST_AUTO_TEST_CASE(BoolContainerWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::BoxWrongEncoding<bond::maybe<std::vector<bool> > > >(
        WireTypeMatchFlag::LengthDelimited | WireTypeMatchFlag::VarInt);
}

BOOST_AUTO_TEST_CASE(BoolUnpackedContainerWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::BoxUnpackedWrongEncoding<bond::maybe<std::vector<bool> > > >(
        WireTypeMatchFlag::LengthDelimited | WireTypeMatchFlag::VarInt);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerContainerWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerUnpackedContainerWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckWireTypeMatch<unittest::BoxUnpacked<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedFixed<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerContainerWireTypeMatchTests, T, signed_integer_types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxZigZag<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerUnpackedContainerWireTypeMatchTests, T, signed_integer_types)
{
    CheckWireTypeMatch<unittest::BoxUnpacked<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedFixed<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedZigZag<bond::maybe<std::vector<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE(BoolUnpackedSetContainerWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::BoxUnpackedWrongEncoding<bond::maybe<std::set<bool> > > >(
        WireTypeMatchFlag::LengthDelimited | WireTypeMatchFlag::VarInt);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerSetContainerWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerUnpackedSetContainerWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckWireTypeMatch<unittest::BoxUnpacked<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedFixed<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
}

using SignedIntegerSetContainerWireTypeMatchTests_Types =
    expand<signed_integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerSetContainerWireTypeMatchTests, T,
    SignedIntegerSetContainerWireTypeMatchTests_Types)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxFixed<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxZigZag<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
}

using SignedIntegerUnpackedSetContainerWireTypeMatchTests_Types =
    expand<signed_integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerUnpackedSetContainerWireTypeMatchTests, T,
    SignedIntegerUnpackedSetContainerWireTypeMatchTests_Types)
{
    CheckWireTypeMatch<unittest::BoxUnpacked<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedFixed<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
    CheckWireTypeMatch<unittest::BoxUnpackedZigZag<bond::maybe<std::set<T> > > >(WireTypeMatchFlag::All);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringContainerWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<std::vector<T> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringSetContainerWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<std::set<T> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(StructContainerWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::vector<unittest::Box<uint32_t> > > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(StructWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<unittest::Box<uint32_t> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerMapKeyWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckMapKeyWireTypeMatch<unittest::Box<std::map<T, uint32_t> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapKeyWireTypeMatch<unittest::BoxFixedKey<T, uint32_t> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

using SignedIntegerMapKeyWireTypeMatchTests_Types = expand<signed_integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerMapKeyWireTypeMatchTests, T,
    SignedIntegerMapKeyWireTypeMatchTests_Types)
{
    CheckMapKeyWireTypeMatch<unittest::Box<std::map<T, uint32_t> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapKeyWireTypeMatch<unittest::BoxFixedKey<T, uint32_t> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapKeyWireTypeMatch<unittest::BoxZigZagKey<T, uint32_t> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapKeyWireTypeMatchTests, T, string_types)
{
    CheckMapKeyWireTypeMatch<unittest::BoxWrongPackingWrongKeyEncoding<std::map<T, uint32_t> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(UnsignedIntegerMapValueWireTypeMatchTests, T, unsigned_integer_types)
{
    CheckMapValueWireTypeMatch<unittest::Box<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapValueWireTypeMatch<unittest::BoxFixedValue<uint32_t, T> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

using SignedIntegerMapValueWireTypeMatchTests_Types = expand<signed_integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SignedIntegerMapValueWireTypeMatchTests, T,
    SignedIntegerMapValueWireTypeMatchTests_Types)
{
    CheckMapValueWireTypeMatch<unittest::Box<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapValueWireTypeMatch<unittest::BoxFixedValue<uint32_t, T> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
    CheckMapValueWireTypeMatch<unittest::BoxZigZagValue<uint32_t, T> >(
        WireTypeMatchFlag::VarInt | WireTypeMatchFlag::Fixed32 | WireTypeMatchFlag::Fixed64);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapValueWireTypeMatchTests, T, string_types)
{
    CheckMapValueWireTypeMatch<unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobMapValueWireTypeMatchTests, T, blob_types)
{
    CheckMapValueWireTypeMatch<unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(StructMapValueWireTypeMatchTests)
{
    CheckMapValueWireTypeMatch<unittest::Box<std::map<uint32_t, unittest::Box<uint32_t> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_SUITE_END()
