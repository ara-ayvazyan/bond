#include "precompiled.h"
#include "protobuf_reader_test_utils.h"
#include "append_to.h"
#include "protobuf_writer_reflection.h"

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <initializer_list>
#include <random>


BOOST_AUTO_TEST_SUITE(ProtobufReaderFormatTests)

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
        bond::InputBuffer input{ merged_payload.data(), static_cast<uint32_t>(merged_payload.length()) };
        bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input, strict };

        // Compile-time schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader)));
        // Runtime schema
        BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>())));
    }
    // merged object
    {
        
        bond::InputBuffer input{ payload.data(), static_cast<uint32_t>(payload.length()) };
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
    CheckBinaryFormat<Proto>(std::initializer_list<Bond>{ Bond{} }, strict);

    auto s1 = InitRandom<Bond>();
    CheckBinaryFormat<Proto>(std::initializer_list<Bond>{ s1 }, strict);

    auto s2 = InitRandom<Bond>();
    auto s3 = InitRandom<Bond>();
    CheckBinaryFormat<Proto>(std::initializer_list<Bond>{ s1, s2, s3, s2, s1, s3 }, strict);
}

BOOST_AUTO_TEST_CASE(IntegerTests)
{
    CheckBinaryFormat<unittest::proto::Integers, unittest::Integers>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringTests, T, string_types)
{
    CheckBinaryFormat<google::protobuf::StringValue, unittest::BoxWrongPackingWrongEncoding<T> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobTests, T, blob_types)
{
    CheckBinaryFormat<google::protobuf::BytesValue, unittest::BoxWrongPackingWrongEncoding<T> >();
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
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringContainerTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<T> > >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringSetContainerTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<T> > >();
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
    CheckBinaryFormat<unittest::proto::BlobContainer>(std::initializer_list<decltype(box)>{ box });
}

BOOST_AUTO_TEST_CASE(StructContainerTests)
{
    CheckBinaryFormat<
        unittest::proto::StructContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<unittest::IntegersContainer> > >();
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

BOOST_AUTO_TEST_CASE(NullableTests)
{
    CheckBinaryFormat<unittest::proto::Nullable, unittest::Nullable>();
}

BOOST_AUTO_TEST_CASE(IntegerMapKeyTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapKeyTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::BoxWrongPackingWrongKeyEncoding<std::map<T, uint32_t> > >();
}

BOOST_AUTO_TEST_CASE(IntegerMapValueTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapValues, unittest::IntegerMapValues>();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StringMapValueTests, T, string_types)
{
    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, T> > >();
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
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, unittest::IntegersContainer> > >();
}

BOOST_AUTO_TEST_CASE(ComplexStructTests)
{
    CheckBinaryFormat<unittest::proto::ComplexStruct, unittest::ComplexStruct>();
}

BOOST_AUTO_TEST_CASE(FieldOrderingTests)
{
    using Bond = unittest::Integers;
    using Proto = unittest::proto::Integers;

    auto bond_struct = InitRandom<Bond>();
    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    std::array<int, boost::mpl::size<Bond::Schema::fields>::value> field_ids;
    {
        auto desc = proto_struct.GetDescriptor();
        for (int i = 0; i < desc->field_count(); ++i)
        {
            field_ids[i] = desc->field(i)->number();
        }
    }

    {
        std::random_device rd;
        std::shuffle(field_ids.begin(), field_ids.end(), std::mt19937{ rd() });
    }

    google::protobuf::string payload;

    for (int id : field_ids)
    {
        auto proto_struct_field = proto_struct;
        auto desc = proto_struct_field.GetDescriptor();
        auto refl = proto_struct_field.GetReflection();

        for (int i = 0; i < desc->field_count(); ++i)
        {
            auto field = desc->field(i);
            if (field->number() != id)
            {
                refl->ClearField(&proto_struct_field, field);
            }
        }

        payload += proto_struct_field.SerializeAsString();
    }

    bond::InputBuffer input{ payload.data(), static_cast<uint32_t>(payload.length()) };
    bond::ProtobufBinaryReader<bond::InputBuffer> reader{ input };

    // Compile-time schema
    BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader)));
    // Runtime schema
    BOOST_CHECK((bond_struct == bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>())));
}

BOOST_AUTO_TEST_SUITE_END()
