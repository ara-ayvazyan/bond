#include "precompiled.h"
#include "protobuf_reader_test_utils.h"
#include "protobuf_writer_reflection.h"

#include <boost/mpl/map.hpp>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(ProtobufReaderUnsupportedTypesTests)

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

template <typename Bond, typename Proto, bool RuntimeOnly = false>
void CheckUnsupportedValueType()
{
    Proto value;
    SetValue(value);
    CheckUnsupportedType<Bond, RuntimeOnly>(value);
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

using IntegerWrongEncodingTests_Types = expand<integer_types, int8_t>;
BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerWrongEncodingTests, T, IntegerWrongEncodingTests_Types)
{
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<T>, proto_box<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(IntegerUnsignedZigZagTests, T, unsigned_integer_types)
{
    CheckUnsupportedValueType<unittest::BoxZigZag<T>, proto_box<T> >();
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

BOOST_AUTO_TEST_CASE(FloatPointMapKeyTests)
{
    auto bond_struct = InitRandom<unittest::IntegersContainer>();
    unittest::proto::NestedStruct proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ *proto_struct.mutable_value() }, bond_struct);

    CheckUnsupportedType<unittest::Box<std::map<float, uint32_t> >, true>(proto_struct);
    CheckUnsupportedType<unittest::Box<std::map<double, uint32_t> >, true>(proto_struct);
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

BOOST_AUTO_TEST_SUITE_END()
