#include "precompiled.h"
#include "unit_test_util.h"
#include "to_proto.h"

#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"
#include "protobuf_writer.pb.h"

#include <bond/core/tuple.h>
#include <bond/protocol/protobuf_binary_reader.h>

#include <google/protobuf/util/message_differencer.h>

#include <boost/test/debug.hpp>
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(ProtobufReaderTests)

template <typename Proto, typename Bond>
void CheckBinaryFormat(const Bond& bond_struct)
{
    Proto proto_struct;
    bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

    auto str = proto_struct.SerializeAsString();

    bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));
    bond::ProtobufBinaryReader<bond::InputBuffer> reader(input);
    auto bond_struct2 = bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>());
    BOOST_CHECK((bond_struct == bond_struct2));
}

template <typename Proto, typename Bond>
void CheckBinaryFormat()
{
    CheckBinaryFormat<Proto>(InitRandom<Bond>());
}

BOOST_AUTO_TEST_CASE(ExperimentTest)
{
    CheckBinaryFormat<unittest::proto::Integers, unittest::Integers>();

    CheckBinaryFormat<unittest::proto::String, unittest::BoxWrongEncoding<std::string> >();
    CheckBinaryFormat<unittest::proto::String, unittest::BoxWrongEncoding<std::wstring> >();

    CheckBinaryFormat<unittest::proto::Blob, unittest::Box<bond::blob> >();
    CheckBinaryFormat<unittest::proto::Blob, unittest::Box<std::vector<int8_t> > >();
    CheckBinaryFormat<unittest::proto::Blob, unittest::Box<bond::nullable<int8_t> > >();

    CheckBinaryFormat<
        unittest::proto::NestedStruct,
        unittest::BoxWrongPackingWrongEncoding<unittest::Integers> >();

    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersContainer>();

    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersSetContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersSetContainer>();

    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<std::string> > >();
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<std::wstring> > >();

    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<std::string> > >();
    CheckBinaryFormat<
        unittest::proto::StringContainer,
        unittest::BoxWrongPackingWrongEncoding<std::set<std::wstring> > >();

    CheckBinaryFormat<
        unittest::proto::BlobContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<bond::blob> > >();
    CheckBinaryFormat<
        unittest::proto::BlobContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<std::vector<int8_t> > > >();
    CheckBinaryFormat<
        unittest::proto::BlobContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<bond::nullable<int8_t> > > >();

    unittest::BoxWrongPackingWrongEncoding<std::vector<bond::blob> > box;
    box.value.resize(2, bond::blob{});
    CheckBinaryFormat<unittest::proto::BlobContainer>(box);

    CheckBinaryFormat<
        unittest::proto::StructContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<unittest::Integers> > >();

    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();

    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::Box<std::map<std::string, uint32_t> > >();
    CheckBinaryFormat<
        unittest::proto::StringMapKey,
        unittest::Box<std::map<std::wstring, uint32_t> > >();

    CheckBinaryFormat<unittest::proto::IntegerMapValues, unittest::IntegerMapValues>();

    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::Box<std::map<uint32_t, std::string> > >();
    CheckBinaryFormat<
        unittest::proto::StringMapValue,
        unittest::Box<std::map<uint32_t, std::wstring> > >();

    CheckBinaryFormat<
        unittest::proto::StructMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, unittest::Integers> > >();

    CheckBinaryFormat<
        unittest::proto::BlobMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, bond::blob> > >();
    CheckBinaryFormat<
        unittest::proto::BlobMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, std::vector<int8_t> > > >();
    CheckBinaryFormat<
        unittest::proto::BlobMapValue,
        unittest::BoxWrongPackingWrongValueEncoding<std::map<uint32_t, bond::nullable<int8_t> > > >();
}

BOOST_AUTO_TEST_SUITE_END()

bool init_unit_test()
{
    boost::debug::detect_memory_leaks(false);
    bond::RandomProtocolReader::Seed(time(nullptr), time(nullptr) / 1000);
    return true;
}
