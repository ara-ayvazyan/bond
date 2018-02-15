#include "precompiled.h"
#include "unit_test_util.h"
#include "to_proto.h"

#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"
#include "protobuf_writer.pb.h"

#include <bond/protocol/protobuf_binary_reader.h>

#include <boost/test/debug.hpp>
#include <boost/test/unit_test.hpp>


namespace bond
{
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
void CheckBinaryFormat(std::initializer_list<Bond> bond_structs)
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
    bond::ProtobufBinaryReader<bond::InputBuffer> reader(input);

    // Compile-time schema
    {
        auto bond_struct2 = bond::Deserialize<Bond>(reader);
        BOOST_CHECK((bond_struct == bond_struct2));
    }

    // Runtime schema
    {
        auto bond_struct2 = bond::Deserialize<Bond>(reader, bond::GetRuntimeSchema<Bond>());
        BOOST_CHECK((bond_struct == bond_struct2));
    }
}

template <typename Proto, typename Bond>
void CheckBinaryFormat()
{
    CheckBinaryFormat<Proto>({ Bond{} });
    CheckBinaryFormat<Proto>({ InitRandom<Bond>() });
    CheckBinaryFormat<Proto>({ InitRandom<Bond>(), InitRandom<Bond>(), InitRandom<Bond>() });
}

template <typename T>
class ToNoUnknowns : public bond::To<T>
{
public:
    ToNoUnknowns(T& var)
        : bond::To<T>(var)
    {}

    template <typename X>
    void UnknownField(uint16_t /*id*/, const X& /*value*/) const
    {
        BOND_THROW(bond::CoreException, "Unknown field");
    }
};

template <typename Bond>
void CheckUnsupportedType()
{
    auto bond_struct = InitRandom<Bond>();

    if (bond_struct != Bond{})
    {
        Proto proto_struct;
        bond::Apply(bond::detail::proto::ToProto{ proto_struct }, bond_struct);

        auto str = proto_struct.SerializeAsString();

        bond::InputBuffer input(str.data(), static_cast<uint32_t>(str.length()));
        bond::ProtobufBinaryReader<bond::InputBuffer> reader(input);

        Bond obj;
        BOOST_CHECK_THROW(
            bond::Apply(
                ToNoUnknowns<Bond>(obj),
                bond::bonded<void, bond::ProtobufBinaryReader<bond::InputBuffer>&>(
                    reader, bond::GetRuntimeSchema<Bond>())),
            bond::CoreException);
    }
}

using blob_types = boost::mpl::list<bond::blob, std::vector<int8_t>, bond::nullable<int8_t> >;

BOOST_AUTO_TEST_CASE(InheritanceTests)
{
    //CheckUnsupportedType<unittest::Derived>();
}

BOOST_AUTO_TEST_CASE(FieldOrdinalTests)
{
    /*CheckUnsupportedType<unittest::Field0>();
    CheckUnsupportedType<unittest::Field19000>();
    CheckUnsupportedType<unittest::Field19111>();
    CheckUnsupportedType<unittest::Field19999>();*/
}

BOOST_AUTO_TEST_CASE(IntegerTests)
{
    CheckBinaryFormat<unittest::proto::Integers, unittest::Integers>();

    /*CheckUnsupportedType<unittest::BoxWrongEncoding<uint8_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<uint16_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<uint32_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<uint64_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<int8_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<int16_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<int32_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<int64_t> >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<unittest::Enum> >();

    CheckUnsupportedType<unittest::BoxZigZag<uint8_t> >();
    CheckUnsupportedType<unittest::BoxZigZag<uint16_t> >();
    CheckUnsupportedType<unittest::BoxZigZag<uint32_t> >();
    CheckUnsupportedType<unittest::BoxZigZag<uint64_t> >();*/
}

BOOST_AUTO_TEST_CASE(StringTests)
{
    CheckBinaryFormat<unittest::proto::String, unittest::BoxWrongPackingWrongEncoding<std::string> >();

    CheckBinaryFormat<unittest::proto::String, unittest::BoxWrongPackingWrongEncoding<std::wstring> >();
}

BOOST_AUTO_TEST_CASE_TEMPLATE(BlobTests, T, blob_types)
{
    CheckBinaryFormat<unittest::proto::Blob, unittest::BoxWrongPackingWrongEncoding<T> >();
}

BOOST_AUTO_TEST_CASE(IntegerContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersContainer>();

    /*CheckUnsupportedType<unittest::BoxWrongEncoding<vector<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<uint64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<int16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<int32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<int64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<vector<unittest::Enum> > >();

    CheckUnsupportedType<unittest::BoxZigZag<vector<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<vector<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<vector<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<vector<uint64_t> > >();

    CheckUnsupportedType<unittest::BoxWrongPacking<vector<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<uint64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<int16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<int32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<int64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<unittest::Enum> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<bool> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<float> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<vector<double> > >();*/
}

BOOST_AUTO_TEST_CASE(IntegerSetContainerTests)
{
    CheckBinaryFormat<unittest::proto::IntegersContainer, unittest::IntegersSetContainer>();
    CheckBinaryFormat<unittest::proto::UnpackedIntegersContainer, unittest::UnpackedIntegersSetContainer>();

    /*CheckUnsupportedType<unittest::BoxWrongEncoding<set<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<uint64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<int8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<int16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<int32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<int64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongEncoding<set<unittest::Enum> > >();

    CheckUnsupportedType<unittest::BoxZigZag<set<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<set<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<set<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxZigZag<set<uint64_t> > >();

    CheckUnsupportedType<unittest::BoxWrongPacking<set<uint8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<uint16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<uint32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<uint64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<int8_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<int16_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<int32_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<int64_t> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<unittest::Enum> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<bool> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<float> > >();
    CheckUnsupportedType<unittest::BoxWrongPacking<set<double> > >();*/
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

    /*unittest::BoxWrongPackingWrongEncoding<bond::bonded<unittest::Integers> > box;
    box.value = bond::bonded<unittest::Integers>{ InitRandom<unittest::Integers>() };
    CheckBinaryFormat<unittest::proto::NestedStruct>(box);*/
}

BOOST_AUTO_TEST_CASE(NullableTests)
{
    CheckBinaryFormat<unittest::proto::Nullable, unittest::Nullable>();
}

BOOST_AUTO_TEST_CASE(IntegerMapKeyTests)
{
    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();

    /*CheckUnsupportedType<unittest::Box<std::map<float, uint32_t> > >();
    CheckUnsupportedType<unittest::Box<std::map<double, uint32_t> > >();*/
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
    /*CheckUnsupportedType<unittest::Box<std::vector<std::vector<uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<bool> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<float> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<double> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<std::wstring> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<bond::blob> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::vector<unittest::Integers> > > >();

    CheckUnsupportedType<unittest::Box<std::vector<std::set<uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<int8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<bool> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<float> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<double> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::set<std::wstring> > > >();

    CheckUnsupportedType<unittest::Box<std::map<uint32_t, std::vector<uint8_t> > > >();
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

    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, uint8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, uint16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, uint32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, uint64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, int8_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, int16_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, int32_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, int64_t> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, unittest::Enum> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, bool> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, float> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, double> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, std::string> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, std::wstring> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, bond::blob> > > >();
    CheckUnsupportedType<unittest::Box<std::vector<std::map<uint32_t, unittest::Integers> > > >();*/
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
