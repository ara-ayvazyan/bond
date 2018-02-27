#include "precompiled.h"
#include "unit_test_util.h"
#include "to_proto.h"
#include "append_to.h"

#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"
#include "protobuf_writer.pb.h"

#include <google/protobuf/wrappers.pb.h>

#include <bond/protocol/protobuf_binary_reader.h>

#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/map.hpp>
#include <boost/range/irange.hpp>
#include <boost/test/debug.hpp>
#include <boost/test/unit_test.hpp>

#include <initializer_list>
#include <set>


namespace bond
{
    template <> struct
    is_protocol_enabled<ProtobufBinaryReader<InputBuffer> >
        : std::true_type {};

} // namespace bond


BOOST_AUTO_TEST_SUITE(ProtobufReaderTests)

template <typename Proto, typename Bond>
void CheckBinaryFormat(std::initializer_list<Bond> bond_structs, bool strict = true)
{
    Proto proto_struct;
    Bond bond_struct;
    google::protobuf::string merged_payload;

    for (const Bond& b : bond_structs)
    {
        Proto p;
        bond::Apply(bond::detail::proto::ToProto{ p }, b);
        proto_struct.MergeFrom(p);
        merged_payload += p.SerializeAsString();
        bond::Apply(bond::detail::proto::AppendTo<Bond>{ b }, bond_struct);
    }

    auto payload = proto_struct.SerializeAsString();

    // merged payload
    {
        bond::InputBuffer input(merged_payload.data(), static_cast<uint32_t>(merged_payload.length()));
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };

        // Compile-time schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader)));
        // Runtime schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>())));
    }
    // merged object
    {
        
        bond::InputBuffer input(payload.data(), static_cast<uint32_t>(payload.length()));
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };

        // Compile-time schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader)));
        // Runtime schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>())));
    }
}

template <typename Proto, typename Bond>
void CheckBinaryFormat(bool strict = true)
{
    CheckBinaryFormat<Proto>({ Bond{} }, strict);

    auto s1 = InitRandom<Bond>();
    CheckBinaryFormat<Proto>({ s1 }, strict);

    auto s2 = InitRandom<Bond>();
    auto s3 = InitRandom<Bond>();
    CheckBinaryFormat<Proto>({ s1, s2, s3, s2, s1, s3 }, strict);
}

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

template <typename Bond>
void DeserializeCheckThrow(
    const bond::ProtobufBinaryReader<bond::InputBuffer>& reader, std::true_type)
{
    BOOST_CHECK_THROW(bond::Deserialize<Bond>(reader), bond::CoreException);
}

template <typename Bond>
void DeserializeCheckThrow(
    const bond::ProtobufBinaryReader<bond::InputBuffer>& /*reader*/, std::false_type)
{}

template <typename Bond, bool RuntimeOnly = false, typename Proto>
void CheckUnsupportedType(const Proto& proto_struct)
{
    auto str = proto_struct.SerializeAsString();

    bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));
    bond::ProtobufBinaryReader<bond::InputBuffer> reader(input);

    // Compile-time schema
    DeserializeCheckThrow<Bond>(reader, std::integral_constant<bool, !RuntimeOnly>{});
    // Runtime schema
    BOOST_CHECK_THROW(
        bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>()),
        bond::CoreException);
}

template <typename Proto, typename Bond, bool RuntimeOnly = false>
void CheckUnsupportedType()
{
    auto bond_struct = InitRandom<Bond>();

    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    CheckUnsupportedType<Bond, RuntimeOnly>(proto_struct);
}

const char* GetMagicBytes()
{
    // The payload will successfully be deserialized for any wire type into non-default value.
    // 1. varint will read single \x8
    // 2. fixed32 will read first 4 bytes or all 8 (when packed)
    // 3. fixed64 will read all 8 bytes
    // 4. lengthdelim will read either the whole content,
    //    or when Box<uint32_t> is used - 4 instances of \x8\x1
    return "\x8\x1\x8\x1\x8\x1\x8\x1";
}

template <typename Proto>
void SetValue(Proto& value)
{
    value.set_value(1);
}

void SetValue(google::protobuf::StringValue& value)
{
    value.set_value("1");
}

void SetValue(google::protobuf::BytesValue& value)
{
    value.set_value(GetMagicBytes());
}

void SetValue(unittest::proto::BlobMapValue& value)
{
    (*value.mutable_value())[1] = "1";
}

template <typename Bond, typename Proto, bool RuntimeOnly = false>
void CheckUnsupportedValueType()
{
    Proto value;
    SetValue(value);
    CheckUnsupportedType<Bond, RuntimeOnly>(value);
}

template <typename List, typename... T>
using expand = boost::mpl::joint_view<List, boost::mpl::list<T...> >;

using blob_types = boost::mpl::list<bond::blob, std::vector<int8_t>, bond::nullable<int8_t> >;
using string_types = boost::mpl::list<std::string, std::wstring>;
using float_point_types = boost::mpl::list<float, double>;
using unsigned_integer_types = boost::mpl::list<uint8_t, uint16_t, uint32_t, uint64_t>;
using signed_integer_types = boost::mpl::list<int16_t, int32_t, int64_t, unittest::Enum>;
using integer_types = expand<unsigned_integer_types, int16_t, int32_t, int64_t, unittest::Enum>;
using basic_types = expand<integer_types, bool, float, double, std::string, std::wstring>;
using scalar_types = expand<basic_types, bond::blob, std::vector<int8_t>, bond::nullable<int8_t> >;

using proto_box_mapping = boost::mpl::map<
    boost::mpl::pair<uint8_t, google::protobuf::UInt32Value>,
    boost::mpl::pair<uint16_t, google::protobuf::UInt32Value>,
    boost::mpl::pair<uint32_t, google::protobuf::UInt32Value>,
    boost::mpl::pair<uint64_t, google::protobuf::UInt64Value>,
    boost::mpl::pair<int8_t, google::protobuf::Int32Value>,
    boost::mpl::pair<int16_t, google::protobuf::Int32Value>,
    boost::mpl::pair<int32_t, google::protobuf::Int32Value>,
    boost::mpl::pair<int64_t, google::protobuf::Int64Value>,
    boost::mpl::pair<unittest::Enum, google::protobuf::Int32Value>,
    boost::mpl::pair<bool, google::protobuf::BoolValue>,
    boost::mpl::pair<float, google::protobuf::FloatValue>,
    boost::mpl::pair<double, google::protobuf::DoubleValue>,
    boost::mpl::pair<bond::blob, google::protobuf::BytesValue>,
    boost::mpl::pair<std::string, google::protobuf::StringValue>,
    boost::mpl::pair<std::wstring, google::protobuf::StringValue>,
    boost::mpl::pair<unittest::IntegersContainer, google::protobuf::BytesValue> >;

template <typename T>
using proto_box = typename boost::mpl::at<proto_box_mapping, T>::type;

enum class WireTypeMatchFlag : uint8_t
{
    None = 0,
    VarInt = 0x1,
    Fixed32 = 0x2,
    Fixed64 = 0x4,
    LengthDelimited = 0x8,
    All = 0xF
};

WireTypeMatchFlag operator|(WireTypeMatchFlag left, WireTypeMatchFlag right)
{
    return static_cast<WireTypeMatchFlag>(uint8_t(left) | uint8_t(right));
}

bool operator&(WireTypeMatchFlag left, WireTypeMatchFlag right)
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

BOOST_AUTO_TEST_CASE(InheritanceTests)
{
    CheckUnsupportedValueType<unittest::Derived, google::protobuf::Int32Value, true>();
}

BOOST_AUTO_TEST_CASE(CorruptedTagTests)
{
    using bond::detail::proto::WireType;
    using Bond = bond::Box<uint32_t>;

    for (uint32_t id : { uint32_t(1), uint32_t((std::numeric_limits<uint16_t>::max)() + 1) })
        for (WireType type : { WireType::VarInt, WireType(7) })
            if (id != 1 || type != WireType::VarInt)
            {
                bond::OutputBuffer output;
                output.WriteVariableUnsigned((uint32_t(id) << 3) | uint32_t(type));
                bond::ProtobufBinaryReader<bond::InputBuffer> reader(output.GetBuffer());

                // Compile-time schema
                BOOST_CHECK_THROW(bond::Deserialize<Bond>(reader), bond::CoreException);
                // Runtime schema
                BOOST_CHECK_THROW(
                    bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>()),
                    bond::CoreException);
            }
}

BOOST_AUTO_TEST_CASE(IntegerTests)
{
    CheckBinaryFormat<unittest::proto::Integers, unittest::Integers>();
}

using IntegerWrongEncodingTests_Types = expand<integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerWrongEncodingTests, T, IntegerWrongEncodingTests_Types)
{
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<T>, proto_box<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerUnsignedZigZagTests, T, unsigned_integer_types)
{
    CheckUnsupportedValueType<unittest::BoxZigZag<T>, proto_box<T> >();
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

BOOST_AUTO_TEST_CASE_TEMPLATE(StringTests, T, string_types)
{
    CheckBinaryFormat<google::protobuf::StringValue, unittest::BoxWrongPackingWrongEncoding<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobTests, T, blob_types)
{
    CheckBinaryFormat<google::protobuf::BytesValue, unittest::BoxWrongPackingWrongEncoding<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobWireTypeMatchTests, T, blob_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<T> > >(
        bond::detail::proto::WireType::LengthDelimited, WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(IntegerContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersContainer>();
    CheckBinaryFormat<
        unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersContainer>();

    CheckBinaryFormat<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersContainer>(false);
    CheckBinaryFormat<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersContainer>(false);

    CheckFieldMatch<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersContainer>(false);
    CheckFieldMatch<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersContainer>(false);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerContainerWrongEncodingTests, T, integer_types)
{
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<T> >, proto_box<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerContainerUnsignedZigZagTests, T, unsigned_integer_types)
{
    CheckUnsupportedValueType<unittest::BoxZigZag<vector<T> >, proto_box<T> >();
}

using IntegerContainerWrongPackingTests_Types = expand<integer_types, bool, float, double>;
BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerContainerWrongPackingTests, T,
    IntegerContainerWrongPackingTests_Types)
{
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<T> >, proto_box<T> >();
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

BOOST_AUTO_TEST_CASE(IntegerSetContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersSetContainer>();
    CheckBinaryFormat<
        unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersSetContainer>();

    CheckBinaryFormat<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersSetContainer>(false);
    CheckBinaryFormat<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersSetContainer>(false);

    CheckFieldMatch<
        unittest::proto::IntegersContainer, unittest::UnpackedIntegersSetContainer>(false);
    CheckFieldMatch<
        unittest::proto::UnpackedIntegersContainer, unittest::IntegersSetContainer>(false);
}

using IntegerSetContainerWrongEncodingTests_Types = expand<integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerSetContainerWrongEncodingTests, T,
    IntegerSetContainerWrongEncodingTests_Types)
{
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<std::set<T> >, proto_box<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerSetContainerUnsignedZigZagTests, T, unsigned_integer_types)
{
    CheckUnsupportedValueType<unittest::BoxZigZag<std::set<T> >, proto_box<T> >();
}

using IntegerSetContainerWrongPackingTests_Types =
    expand<integer_types, int8_t, bool, float, double>;
BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerSetContainerWrongPackingTests, T,
    IntegerSetContainerWrongPackingTests_Types)
{
    CheckUnsupportedValueType<unittest::BoxWrongPacking<std::set<T> >, proto_box<T> >();
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

BOOST_AUTO_TEST_CASE_TEMPLATE(StringContainerTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<T> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringContainerWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<std::vector<T> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringSetContainerTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<T> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringSetContainerWireTypeMatchTests, T, string_types)
{
    CheckWireTypeMatch<unittest::BoxWrongPackingWrongEncoding<bond::maybe<std::set<T> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobContainerTests, T, blob_types)
{
    CheckBinaryFormat<
        unittest::proto::BlobContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<T> > >();

    CheckBinaryFormat<
        unittest::proto::BlobContainer,
        unittest::BoxWrongPackingWrongEncoding<bond::nullable<T> > >();

    unittest::BoxWrongPackingWrongEncoding<std::vector<T> > box;
    box.value.resize(2);
    CheckBinaryFormat<unittest::proto::BlobContainer>({ box });
}

BOOST_AUTO_TEST_CASE(StructContainerTests)
{
    CheckBinaryFormat<
        unittest::proto::StructContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<unittest::IntegersContainer> > >();
}

BOOST_AUTO_TEST_CASE(StructContainerWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<std::vector<unittest::Box<uint32_t> > > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(NestedStructTests)
{
    CheckBinaryFormat<
        unittest::proto::NestedStruct,
        unittest::BoxWrongPackingWrongEncoding<unittest::IntegersContainer> >();

    // bonded<T>
    {
        using Bond = unittest::IntegersContainer;
        using Box = unittest::BoxWrongPackingWrongEncoding<bond::bonded<Bond> >;
        using Protocols = bond::Protocols<bond::ProtobufBinaryReader<bond::InputBuffer> >;

        auto bond_struct = InitRandom<Bond>();

        Box bond_box;
        bond_box.value = bond::bonded<Bond>{ boost::ref(bond_struct) };

        unittest::proto::NestedStruct proto_box;
        bond::Apply(bond::detail::proto::ToProto{ *proto_box.mutable_value() }, bond_struct);

        auto str = proto_box.SerializeAsString();

        bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));
        bond::ProtobufBinaryReader<bond::InputBuffer> reader(input);

        // Compile-time schema
        {
            auto bond_box2 = bond::Deserialize<Box>(reader);
            auto bond_struct2 = bond_box2.value.Deserialize<Bond, Protocols>();
            BOOST_CHECK((bond_struct == bond_struct2));
        }
        // Runtime schema
        {
            auto bond_box2 = bond::Deserialize<Box>(reader, bond::GetRuntimeSchema<Box>());
            auto bond_struct2 = bond_box2.value.Deserialize<Bond, Protocols>();
            BOOST_CHECK((bond_struct == bond_struct2));
        }
    }
}

BOOST_AUTO_TEST_CASE(StructWireTypeMatchTests)
{
    CheckWireTypeMatch<unittest::Box<bond::maybe<unittest::Box<uint32_t> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(NullableTests)
{
    CheckBinaryFormat<unittest::proto::Nullable, unittest::Nullable>();
}

BOOST_AUTO_TEST_CASE(IntegerMapKeyTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();

    auto bond_struct = InitRandom<unittest::IntegersContainer>();
    unittest::proto::NestedStruct proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ *proto_struct.mutable_value() }, bond_struct);

    CheckUnsupportedType<unittest::Box<std::map<float, uint32_t> >, true>(proto_struct);
    CheckUnsupportedType<unittest::Box<std::map<double, uint32_t> >, true>(proto_struct);
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

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapKeyTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::BoxWrongPackingWrongKeyEncoding<std::map<T, uint32_t> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapKeyWireTypeMatchTests, T, string_types)
{
    CheckMapKeyWireTypeMatch<unittest::BoxWrongPackingWrongKeyEncoding<std::map<T, uint32_t> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(IntegerMapValueTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapValues, unittest::IntegerMapValues>();
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

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapValueTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapValueWireTypeMatchTests, T, string_types)
{
    CheckMapValueWireTypeMatch<unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobMapValueTests, T, blob_types)
{
    CheckBinaryFormat<
        unittest::proto::BlobMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobMapValueWireTypeMatchTests, T, blob_types)
{
    CheckMapValueWireTypeMatch<unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >(
        WireTypeMatchFlag::LengthDelimited);
}

BOOST_AUTO_TEST_CASE(StructMapValueTests)
{
    CheckBinaryFormat<
        unittest::proto::StructMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, unittest::IntegersContainer> > >();
}

BOOST_AUTO_TEST_CASE(StructMapValueWireTypeMatchTests)
{
    CheckMapValueWireTypeMatch<unittest::Box<std::map<uint32_t, unittest::Box<uint32_t> > > >(
        WireTypeMatchFlag::LengthDelimited);
}

using NestedVectorVectorTests_Types = expand<basic_types, bond::blob, unittest::IntegersContainer>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedVectorVectorTests, T, NestedVectorVectorTests_Types)
{
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<T> > >, proto_box<T>, true>();
}

using NestedVectorSetTests_Types = expand<basic_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedVectorSetTests, T, NestedVectorSetTests_Types)
{
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<T> > >, proto_box<T>, true>();
}

using NestedVectorMapTests_Types = expand<basic_types, int8_t, bond::blob, unittest::IntegersContainer>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedVectorMapTests, T, NestedVectorMapTests_Types)
{
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, T> > >, proto_box<T>, true>();
}

using NestedMapVectorTests_Types = expand<basic_types, bond::blob, unittest::IntegersContainer>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedMapVectorTests, T, NestedMapVectorTests_Types)
{
    CheckUnsupportedValueType<
        unittest::Box<std::map<uint32_t, std::vector<T> > >, unittest::proto::BlobMapValue, true>();
}

using NestedMapSetTests_Types = expand<basic_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedMapSetTests, T, NestedMapSetTests_Types)
{
    CheckUnsupportedValueType<
        unittest::Box<std::map<uint32_t, std::set<T> > >, unittest::proto::BlobMapValue, true>();
}

using NestedMapMapTests_Types = expand<basic_types, int8_t, bond::blob, unittest::IntegersContainer>;
BOOST_AUTO_TEST_CASE_TEMPLATE(NestedMapMapTests, T, NestedMapMapTests_Types)
{
    CheckUnsupportedValueType<
        unittest::Box<std::map<uint32_t, std::map<uint32_t, T> > >, unittest::proto::BlobMapValue, true>();
}

BOOST_AUTO_TEST_CASE(ComplexStructTests)
{
    CheckBinaryFormat<unittest::proto::ComplexStruct, unittest::ComplexStruct>();
}


template <typename T, typename NonMatchingT, typename Proto>
void CheckVarTypeMismatch()
{
    auto bond_struct = InitRandom<unittest::BoxMatchingField<T> >();

    unittest::BoxMatchingField<NonMatchingT> bond_struct_expected;
    bond_struct_expected.m1 = bond_struct.m1;
    bond_struct_expected.m2 = bond_struct.m2;

    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct, true }, bond_struct);

    auto payload = proto_struct.SerializeAsString();
    bond::InputBuffer input{ payload.data(), uint32_t(payload.size()) };

    // Compile-time schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input };
        unittest::BoxMatchingField<NonMatchingT> bond_struct_no_match;
        bond::Apply(
            bond::To<decltype(bond_struct_no_match)>{ bond_struct_no_match },
            bond::bonded<decltype(bond_struct), decltype(reader)&>{ reader });
        BOOST_CHECK((bond_struct_no_match == bond_struct_expected));
    }
    // Runtimetime schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input };
        unittest::BoxMatchingField<NonMatchingT> bond_struct_no_match;
        bond::Apply(
            bond::To<decltype(bond_struct_no_match)>{ bond_struct_no_match },
            bond::bonded<void, decltype(reader)&>{ reader, bond::GetRuntimeSchema<decltype(bond_struct)>() });
        BOOST_CHECK((bond_struct_no_match == bond_struct_expected));
    }
}

using non_matching_bond_mapping = boost::mpl::map<
    boost::mpl::pair<uint8_t, int8_t>,
    boost::mpl::pair<uint16_t, uint8_t>,
    boost::mpl::pair<uint32_t, uint16_t>,
    boost::mpl::pair<uint64_t, uint32_t>,
    boost::mpl::pair<int8_t, uint8_t>,
    boost::mpl::pair<int16_t, int8_t>,
    boost::mpl::pair<int32_t, int16_t>,
    boost::mpl::pair<int64_t, int32_t>,
    boost::mpl::pair<unittest::Enum, int16_t>,
    boost::mpl::pair<bool, int16_t>,
    boost::mpl::pair<float, int32_t>,
    boost::mpl::pair<double, float>,
    boost::mpl::pair<std::string, std::wstring>,
    boost::mpl::pair<std::wstring, std::string>,
    boost::mpl::pair<bond::blob, std::vector<uint8_t> >,
    boost::mpl::pair<std::vector<int8_t>, std::vector<uint8_t> >,
    boost::mpl::pair<bond::nullable<int8_t>, std::vector<uint8_t> >,
    boost::mpl::pair<unittest::Integers, std::string> >;

using matching_proto_mapping = boost::mpl::map<
    boost::mpl::pair<uint8_t, unittest::proto::MatchingUInt32>,
    boost::mpl::pair<uint16_t, unittest::proto::MatchingUInt32>,
    boost::mpl::pair<uint32_t, unittest::proto::MatchingUInt32>,
    boost::mpl::pair<uint64_t, unittest::proto::MatchingUInt64>,
    boost::mpl::pair<int8_t, unittest::proto::MatchingInt32>,
    boost::mpl::pair<int16_t, unittest::proto::MatchingInt32>,
    boost::mpl::pair<int32_t, unittest::proto::MatchingInt32>,
    boost::mpl::pair<int64_t, unittest::proto::MatchingInt64>,
    boost::mpl::pair<unittest::Enum, unittest::proto::MatchingEnum>,
    boost::mpl::pair<bool, unittest::proto::MatchingBool>,
    boost::mpl::pair<float, unittest::proto::MatchingFloat>,
    boost::mpl::pair<double, unittest::proto::MatchingDouble>,
    boost::mpl::pair<std::string, unittest::proto::MatchingString>,
    boost::mpl::pair<std::wstring, unittest::proto::MatchingString>,
    boost::mpl::pair<bond::blob, unittest::proto::MatchingBlob>,
    boost::mpl::pair<std::vector<int8_t>, unittest::proto::MatchingBlob>,
    boost::mpl::pair<bond::nullable<int8_t>, unittest::proto::MatchingBlob>,
    boost::mpl::pair<unittest::Integers, unittest::proto::MatchingStruct> >;

using matching_proto_map_key_mapping = boost::mpl::map<
    boost::mpl::pair<uint8_t, unittest::proto::MatchingMapKeyUInt32>,
    boost::mpl::pair<uint16_t, unittest::proto::MatchingMapKeyUInt32>,
    boost::mpl::pair<uint32_t, unittest::proto::MatchingMapKeyUInt32>,
    boost::mpl::pair<uint64_t, unittest::proto::MatchingMapKeyUInt64>,
    boost::mpl::pair<int8_t, unittest::proto::MatchingMapKeyInt32>,
    boost::mpl::pair<int16_t, unittest::proto::MatchingMapKeyInt32>,
    boost::mpl::pair<int32_t, unittest::proto::MatchingMapKeyInt32>,
    boost::mpl::pair<int64_t, unittest::proto::MatchingMapKeyInt64>,
    boost::mpl::pair<bool, unittest::proto::MatchingMapKeyBool>,
    boost::mpl::pair<std::string, unittest::proto::MatchingMapKeyString>,
    boost::mpl::pair<std::wstring, unittest::proto::MatchingMapKeyString> >;

using matching_proto_map_value_mapping = boost::mpl::map<
    boost::mpl::pair<uint8_t, unittest::proto::MatchingMapValueUInt32>,
    boost::mpl::pair<uint16_t, unittest::proto::MatchingMapValueUInt32>,
    boost::mpl::pair<uint32_t, unittest::proto::MatchingMapValueUInt32>,
    boost::mpl::pair<uint64_t, unittest::proto::MatchingMapValueUInt64>,
    boost::mpl::pair<int8_t, unittest::proto::MatchingMapValueInt32>,
    boost::mpl::pair<int16_t, unittest::proto::MatchingMapValueInt32>,
    boost::mpl::pair<int32_t, unittest::proto::MatchingMapValueInt32>,
    boost::mpl::pair<int64_t, unittest::proto::MatchingMapValueInt64>,
    boost::mpl::pair<unittest::Enum, unittest::proto::MatchingMapValueEnum>,
    boost::mpl::pair<bool, unittest::proto::MatchingMapValueBool>,
    boost::mpl::pair<float, unittest::proto::MatchingMapValueFloat>,
    boost::mpl::pair<double, unittest::proto::MatchingMapValueDouble>,
    boost::mpl::pair<std::string, unittest::proto::MatchingMapValueString>,
    boost::mpl::pair<std::wstring, unittest::proto::MatchingMapValueString>,
    boost::mpl::pair<bond::blob, unittest::proto::MatchingMapValueBlob>,
    boost::mpl::pair<std::vector<int8_t>, unittest::proto::MatchingMapValueBlob>,
    boost::mpl::pair<bond::nullable<int8_t>, unittest::proto::MatchingMapValueBlob>,
    boost::mpl::pair<unittest::Integers, unittest::proto::MatchingMapValueStruct> >;

template <typename T>
using non_matching_bond = typename boost::mpl::at<non_matching_bond_mapping, T>::type;

template <typename T>
using matching_proto = typename boost::mpl::at<matching_proto_mapping, T>::type;

template <typename T>
using matching_proto_map_key = typename boost::mpl::at<matching_proto_map_key_mapping, T>::type;

template <typename T>
using matching_proto_map_value = typename boost::mpl::at<matching_proto_map_value_mapping, T>::type;


template <typename T>
typename boost::disable_if_c<(bond::is_list_container<T>::value || bond::is_set_container<T>::value)
                            && !bond::detail::proto::is_blob_type<T>::value>::type
CheckVarTypeMismatch()
{
    CheckVarTypeMismatch<T, non_matching_bond<T>, matching_proto<T> >();
}

template <typename T>
typename boost::enable_if_c<(bond::is_list_container<T>::value || bond::is_set_container<T>::value)
                            && !bond::detail::proto::is_blob_type<T>::value>::type
CheckVarTypeMismatch()
{
    using element = typename bond::element_type<T>::type;

    CheckVarTypeMismatch<T, non_matching_bond<element>, matching_proto<element> >();
}


template <typename T>
typename boost::enable_if<bond::is_map_container<T> >::type
CheckMapVarTypeKeyMismatch()
{
    using key_type = typename bond::element_type<T>::type::first_type;

    CheckVarTypeMismatch<T, non_matching_bond<key_type>, matching_proto_map_key<key_type> >();
}

template <typename T>
typename boost::enable_if<bond::is_map_container<T> >::type
CheckMapVarTypeValueMismatch()
{
    using value_type = typename bond::element_type<T>::type::second_type;

    CheckVarTypeMismatch<T, non_matching_bond<value_type>, matching_proto_map_value<value_type> >();
}


using ScalarVarTypeMismatchTests_Types = expand<scalar_types, int8_t, unittest::Integers>;
BOOST_AUTO_TEST_CASE_TEMPLATE(ScalarVarTypeMismatchTests, T, ScalarVarTypeMismatchTests_Types)
{
    CheckVarTypeMismatch<T>();
}

using ContainerVarTypeMismatchTests_Types = expand<scalar_types, unittest::Integers>;
BOOST_AUTO_TEST_CASE_TEMPLATE(ContainerVarTypeMismatchTests, T, ContainerVarTypeMismatchTests_Types)
{
    CheckVarTypeMismatch<std::vector<T> >();
}

using SetContainerVarTypeMismatchTests_Types = expand<basic_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(SetContainerVarTypeMismatchTests, T, SetContainerVarTypeMismatchTests_Types)
{
    CheckVarTypeMismatch<std::set<T> >();
}

using MapVarTypeKeyMismatchTests_Types = expand<unsigned_integer_types,
    bool, int8_t, int16_t, int32_t, int64_t, std::string, std::wstring>;
BOOST_AUTO_TEST_CASE_TEMPLATE(MapVarTypeKeyMismatchTests, T, MapVarTypeKeyMismatchTests_Types)
{
    CheckMapVarTypeKeyMismatch<std::map<T, uint32_t> >();
}

using MapVarTypeValueMismatchTests_Types = expand<scalar_types, int8_t, unittest::Integers>;
BOOST_AUTO_TEST_CASE_TEMPLATE(MapVarTypeValueMismatchTests, T, MapVarTypeValueMismatchTests_Types)
{
    CheckMapVarTypeValueMismatch<std::map<uint32_t, T> >();
}

BOOST_AUTO_TEST_SUITE_END()

bool init_unit_test()
{
    boost::debug::detect_memory_leaks(false);
    bond::RandomProtocolReader::Seed(time(nullptr), time(nullptr) / 1000);
    return true;
}
