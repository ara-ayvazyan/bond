#include "precompiled.h"
#include "protobuf_reader_test_utils.h"
#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(ProtobufReaderVarTypeMismatchTests)

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
