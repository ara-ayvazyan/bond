#include "precompiled.h"
#include "unit_test_util.h"
#include "to_proto.h"

#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"
#include "protobuf_writer.pb.h"

#include <google/protobuf/wrappers.pb.h>

#include <bond/protocol/protobuf_binary_reader.h>

#include <boost/range/irange.hpp>
#include <boost/test/debug.hpp>
#include <boost/test/unit_test.hpp>

#include <set>


namespace bond
{

template <> struct
is_protocol_enabled<ProtobufBinaryReader<InputBuffer> >
    : std::true_type {};

namespace detail
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
        bool Base(const X& value) const
        {
            BOOST_ASSERT(false);
        }

        template <typename FieldT, typename X>
        bool Field(const FieldT&, X& value) const
        {
            auto&& var = FieldT::GetVariable(_var);

            if (!detail::is_default(var, FieldT::metadata))
            {
                Append(var, value);
            }

            return false;
        }

    private:
        template <typename X>
        typename boost::enable_if<is_basic_type<X> >::type
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
        typename boost::enable_if_c<is_list_container<X>::value || is_set_container<X>::value>::type
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
                auto item = items.next();
                Append(item.second, mapped_at(value, item.first));
            }
        }

        void Append(const blob& var, blob& value) const
        {
            value = merge(value, var);
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

} // namespace detail
} // namespace bond


BOOST_AUTO_TEST_SUITE(ProtobufReaderTests)

template <typename Proto, typename Bond>
void CheckBinaryFormat(std::initializer_list<Bond> bond_structs, bool strict = true)
{
    google::protobuf::string str;
    Bond bond_struct;

    for (const Bond& s : bond_structs)
    {
        Proto proto_struct;
        bond::Apply(bond::detail::proto::ToProto{ proto_struct }, s);
        str += proto_struct.SerializeAsString();
        bond::Apply(bond::detail::AppendTo<Bond>{ s }, bond_struct);
    }

    bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));
    bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };

    // Compile-time schema
    BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader)));
    // Runtime schema
    BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>())));
}

template <typename Proto, typename Bond>
void CheckBinaryFormat(bool strict = true)
{
    CheckBinaryFormat<Proto>({ Bond{} }, strict);
    CheckBinaryFormat<Proto>({ InitRandom<Bond>() }, strict);
    CheckBinaryFormat<Proto>({ InitRandom<Bond>(), InitRandom<Bond>(), InitRandom<Bond>() }, strict);
}

class SaveUnknownFields : public bond::DeserializingTransform
{
public:
    explicit SaveUnknownFields(boost::shared_ptr<std::set<uint16_t> > ids)
        : _ids{ ids }
    {}

    void Begin(const bond::Metadata& /*metadata*/) const
    {
        _ids->clear();
    }

    void End() const
    {}

    template <typename T>
    bool Field(uint16_t /*id*/, const bond::Metadata& /*metadata*/, const T& /*value*/) const
    {
        return false;
    }

    template <typename T>
    void UnknownField(uint16_t id, const T& /*value*/) const
    {
        _ids->insert(id);
    }

private:
    boost::shared_ptr<std::set<uint16_t> > _ids;
};

template <typename Proto, typename Bond, typename Ids>
void CheckUnknownFields(Ids&& expected_ids)
{
    auto bond_struct = InitRandom<Bond>();

    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    auto str = proto_struct.SerializeAsString();
    bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));

    auto ids = boost::make_shared<std::set<uint16_t> >();

    // Compile-time schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input };
        bond::Apply(
            SaveUnknownFields{ ids },
            bond::bonded<Bond, decltype(reader)&>{ reader });
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(expected_ids), std::end(expected_ids), ids->begin(), ids->end());
    }

    // Runtime schema
    {
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input };
        bond::Apply(
            SaveUnknownFields{ ids },
            bond::bonded<void, decltype(reader)&>{ reader, bond::GetRuntimeSchema<Bond>() });
        BOOST_CHECK_EQUAL_COLLECTIONS(std::begin(expected_ids), std::end(expected_ids), ids->begin(), ids->end());
    }
}

template <typename Bond>
void DeserializeCheckThrow(const bond::ProtobufBinaryReader<bond::InputBuffer>& reader, std::true_type)
{
    BOOST_CHECK_THROW(bond::Deserialize<Bond>(reader), bond::CoreException);
}

template <typename Bond>
void DeserializeCheckThrow(const bond::ProtobufBinaryReader<bond::InputBuffer>& /*reader*/, std::false_type)
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
    BOOST_CHECK_THROW(bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>()), bond::CoreException);
}

template <typename Proto, typename Bond, bool RuntimeOnly = false>
void CheckUnsupportedType()
{
    auto bond_struct = InitRandom<Bond>();

    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    CheckUnsupportedType<Bond, RuntimeOnly>(proto_struct);
}

template <typename Proto>
void set_value(Proto& value)
{
    value.set_value(1);
}

void set_value(google::protobuf::StringValue& value)
{
    value.set_value("1");
}

void set_value(google::protobuf::BytesValue& value)
{
    value.set_value("1");
}

template <typename Bond, typename Proto, bool RuntimeOnly = false>
void CheckUnsupportedValueType()
{
    Proto value;
    set_value(value);
    CheckUnsupportedType<Bond, RuntimeOnly>(value);
}

using blob_types = boost::mpl::list<bond::blob, std::vector<int8_t>, bond::nullable<int8_t> >;

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
                BOOST_CHECK_THROW(bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>()), bond::CoreException);
            }
}

BOOST_AUTO_TEST_CASE(IntegerTests)
{
    CheckBinaryFormat<unittest::proto::Integers, unittest::Integers>();

    CheckUnsupportedValueType<unittest::BoxWrongEncoding<uint8_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<uint16_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<uint32_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<uint64_t>, google::protobuf::UInt64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<int8_t>, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<int16_t>, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<int32_t>, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<int64_t>, google::protobuf::Int64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<unittest::Enum>, google::protobuf::Int32Value>();

    CheckUnsupportedValueType<unittest::BoxZigZag<uint8_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<uint16_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<uint32_t>, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<uint64_t>, google::protobuf::UInt64Value>();
}

BOOST_AUTO_TEST_CASE(StringTests)
{
    CheckBinaryFormat<google::protobuf::StringValue, unittest::BoxWrongPackingWrongEncoding<std::string> >();

    CheckBinaryFormat<google::protobuf::StringValue, unittest::BoxWrongPackingWrongEncoding<std::wstring> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobTests, T, blob_types)
{
    CheckBinaryFormat<google::protobuf::BytesValue, unittest::BoxWrongPackingWrongEncoding<T> >();
}

BOOST_AUTO_TEST_CASE(IntegerContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersContainer>();

    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::UnpackedIntegersContainer>(false);
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::IntegersContainer>(false);

    auto expected_ids = boost::irange<uint16_t>(1, 1 + boost::mpl::size<unittest::IntegersContainer::Schema::fields>::value);
    CheckUnknownFields<unittest::proto::IntegersContainer, unittest::UnpackedIntegersContainer>(expected_ids);
    CheckUnknownFields<unittest::proto::UnpackedIntegersContainer, unittest::IntegersContainer>(expected_ids);

    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<uint64_t> >, google::protobuf::UInt64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<int16_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<int32_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<int64_t> >, google::protobuf::Int64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<vector<unittest::Enum> >, google::protobuf::Int32Value>();

    CheckUnsupportedValueType<unittest::BoxZigZag<vector<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<vector<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<vector<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<vector<uint64_t> >, google::protobuf::UInt64Value>();

    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<uint64_t> >, google::protobuf::UInt64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<int16_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<int32_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<int64_t> >, google::protobuf::Int64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<unittest::Enum> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<bool> >, google::protobuf::BoolValue>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<float> >, google::protobuf::FloatValue>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<vector<double> >, google::protobuf::DoubleValue>();
}

BOOST_AUTO_TEST_CASE(IntegerSetContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersSetContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersSetContainer>();

    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<uint64_t> >, google::protobuf::UInt64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<int8_t> >, google::protobuf::BytesValue>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<int16_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<int32_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<int64_t> >, google::protobuf::Int64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongEncoding<set<unittest::Enum> >, google::protobuf::Int32Value>();

    CheckUnsupportedValueType<unittest::BoxZigZag<set<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<set<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<set<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxZigZag<set<uint64_t> >, google::protobuf::UInt64Value>();

    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<uint8_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<uint16_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<uint32_t> >, google::protobuf::UInt32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<uint64_t> >, google::protobuf::UInt64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<int8_t> >, google::protobuf::BytesValue>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<int16_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<int32_t> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<int64_t> >, google::protobuf::Int64Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<unittest::Enum> >, google::protobuf::Int32Value>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<bool> >, google::protobuf::BoolValue>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<float> >, google::protobuf::FloatValue>();
    CheckUnsupportedValueType<unittest::BoxWrongPacking<set<double> >, google::protobuf::DoubleValue>();
}

BOOST_AUTO_TEST_CASE(StringContainerTests)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<std::string> > >();

    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<std::wstring> > >();
}

BOOST_AUTO_TEST_CASE(StringSetContainerTests)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<std::string> > >();

    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<std::wstring> > >();
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
        unittest::BoxWrongPackingWrongEncoding<std::vector<unittest::Integers> > >();
}

BOOST_AUTO_TEST_CASE(NestedStructTests)
{
    CheckBinaryFormat<
        unittest::proto::NestedStruct,
        unittest::BoxWrongPackingWrongEncoding<unittest::Integers> >();

    // bonded<T>
    {
        using Bond = unittest::Integers;
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

BOOST_AUTO_TEST_CASE(NullableTests)
{
    CheckBinaryFormat<unittest::proto::Nullable, unittest::Nullable>();
}

BOOST_AUTO_TEST_CASE(IntegerMapKeyTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();

    auto bond_struct = InitRandom<unittest::Integers>();
    unittest::proto::NestedStruct proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ *proto_struct.mutable_value() }, bond_struct);

    CheckUnsupportedType<unittest::Box<std::map<float, uint32_t> >, true>(proto_struct);
    CheckUnsupportedType<unittest::Box<std::map<double, uint32_t> >, true>(proto_struct);
}

BOOST_AUTO_TEST_CASE(StringMapKeyTests)
{
    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::BoxWrongPackingWrongKeyEncoding<std::map<std::string, uint32_t> > >();

    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::BoxWrongPackingWrongKeyEncoding<std::map<std::wstring, uint32_t> > >();
}

BOOST_AUTO_TEST_CASE(IntegerMapValueTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapValues, unittest::IntegerMapValues>();
}

BOOST_AUTO_TEST_CASE(StringMapValueTests)
{
    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, std::string> > >();

    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, std::wstring> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobMapValueTests, T, blob_types)
{
    CheckBinaryFormat<
        unittest::proto::BlobMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >();
}

BOOST_AUTO_TEST_CASE(StructMapValueTests)
{
    CheckBinaryFormat<
        unittest::proto::StructMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, unittest::Integers> > >();
}

BOOST_AUTO_TEST_CASE(NestedContainersTests)
{
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<uint8_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<uint16_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<uint32_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<uint64_t> > >, google::protobuf::UInt64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<int16_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<int32_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<int64_t> > >, google::protobuf::Int64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<unittest::Enum> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<bool> > >, google::protobuf::BoolValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<float> > >, google::protobuf::FloatValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<double> > >, google::protobuf::DoubleValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<std::string> > >, google::protobuf::StringValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<std::wstring> > >, google::protobuf::StringValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<bond::blob> > >, google::protobuf::BytesValue, true>();
    //CheckUnsupportedValueType<unittest::Box<std::vector<std::vector<unittest::Integers> > > >();

    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<uint8_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<uint16_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<uint32_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<uint64_t> > >, google::protobuf::UInt64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<int8_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<int16_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<int32_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<int64_t> > >, google::protobuf::Int64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<unittest::Enum> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<bool> > >, google::protobuf::BoolValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<float> > >, google::protobuf::FloatValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<double> > >, google::protobuf::DoubleValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<std::string> > >, google::protobuf::StringValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::set<std::wstring> > >, google::protobuf::StringValue, true>();

    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, uint8_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, uint16_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, uint32_t> > >, google::protobuf::UInt32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, uint64_t> > >, google::protobuf::UInt64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, int8_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, int16_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, int32_t> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, int64_t> > >, google::protobuf::Int64Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, unittest::Enum> > >, google::protobuf::Int32Value, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, bool> > >, google::protobuf::BoolValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, float> > >, google::protobuf::FloatValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, double> > >, google::protobuf::DoubleValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, std::string> > >, google::protobuf::StringValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, std::wstring> > >, google::protobuf::StringValue, true>();
    CheckUnsupportedValueType<unittest::Box<std::vector<std::map<uint32_t, bond::blob> > >, google::protobuf::BytesValue, true>();
    //CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, unittest::Integers> > > >();

    /*CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<bool> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<float> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<double> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<std::wstring> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<bond::blob> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<unittest::Integers> > > >();
    
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<int8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<bool> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<float> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<double> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::set<std::wstring> > > >();
    
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, int8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, bool> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, float> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, double> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, std::wstring> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, bond::blob> > > >();
    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::map<uint32_t, unittest::Integers> > > >();*/
}

BOOST_AUTO_TEST_CASE(ComplexStructTests)
{
    CheckBinaryFormat<unittest::proto::ComplexStruct, unittest::ComplexStruct>();
}

BOOST_AUTO_TEST_SUITE_END()

bool init_unit_test()
{
    boost::debug::detect_memory_leaks(false);
    bond::RandomProtocolReader::Seed(time(nullptr), time(nullptr) / 1000);
    return true;
}
