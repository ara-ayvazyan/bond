#include "precompiled.h"
#include "unit_test_util.h"

#include "protobuf_writer_apply.h"
#include "protobuf_writer_reflection.h"
#include "protobuf_writer.pb.h"

#include <bond/core/tuple.h>
#include <bond/protocol/protobuf_binary_reader.h>

#include <google/protobuf/util/message_differencer.h>

#include <boost/test/debug.hpp>
#include <boost/test/unit_test.hpp>

namespace
{
    class ToProto : public bond::SerializingTransform
    {
    public:
        explicit ToProto(google::protobuf::Message& msg)
            : _message{ msg },
              _reflection{ *_message.GetReflection() },
              _descriptor{ *_message.GetDescriptor() }
        {}

        void Begin(const bond::Metadata&) const
        {}

        void End() const
        {}

        void UnknownEnd() const
        {}

        template <typename T>
        bool Base(T&&) const
        {
            BOOST_ASSERT(false);
            return false;
        }

        template <typename T>
        typename boost::disable_if<bond::is_container<T>, bool>::type
        Field(uint16_t id, const bond::Metadata& /*metadata*/, const T& value) const
        {
            SetValue(GetField(id), value);
            return false;
        }

        template <typename T>
        typename boost::enable_if<bond::is_container<T>, bool>::type
        Field(uint16_t id, const bond::Metadata& /*metadata*/, const T& value) const
        {
            const auto& field = GetField(id);
            for (bond::const_enumerator<T> items(value); items.more(); )
            {
                AddValue(field, items.next());
            }
            return false;
        }

        template <typename T, typename Reader>
        typename boost::enable_if<bond::is_basic_type<T>, bool>::type
        Field(uint16_t id, const bond::Metadata& metadata, const bond::value<T, Reader>& value) const
        {
            T data{};
            value.Deserialize(data);
            return Field(id, metadata, data);
        }

        template <typename T>
        bool Field(uint16_t id, const bond::Metadata& /*metadata*/, const bond::nullable<T>& value) const
        {
            if (value.hasvalue())
            {
                const auto& field = GetField(id);
                if (field.is_repeated())
                {
                    AddValue(field, value.value());
                }
                else
                {
                    SetValue(field, value.value());
                }
            }
            return false;
        }

        bool Field(uint16_t id, const bond::Metadata& /*metadata*/, const bond::blob& value) const
        {
            SetValue(GetField(id), value);
            return false;
        }

        template <typename T>
        bool UnknownField(uint16_t /*id*/, const T& /*value*/) const
        {
            BOOST_ASSERT(false); // Not implemented
            return false;
        }

        template <typename T>
        void Container(const T& /*element*/, uint32_t /*size*/) const
        {
            BOOST_ASSERT(false); // Not implemented
        }

        template <typename Key, typename T>
        void Container(const Key& /*key*/, const T& /*value*/, uint32_t /*size*/) const
        {
            BOOST_ASSERT(false); // Not implemented
        }

    private:
        const google::protobuf::FieldDescriptor& GetField(uint16_t id) const
        {
            auto field = _descriptor.FindFieldByNumber(id);
            BOOST_ASSERT(field);
            return *field;
        }

        template <typename T1, typename T2>
        void AddValue(const google::protobuf::FieldDescriptor& field, const std::pair<T1, T2>& value) const
        {
            AddValue(field, std::forward_as_tuple(std::ignore, value.first, value.second));
        }

        template <typename T>
        typename boost::enable_if<bond::is_bond_type<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            auto msg = _reflection.MutableMessage(&_message, &field);
            BOOST_ASSERT(msg);
            bond::Apply(ToProto{ *msg }, value);
        }

        template <typename T, typename Reader>
        typename boost::disable_if<bond::is_basic_type<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, const bond::value<T, Reader>& value) const
        {
            auto msg = _reflection.MutableMessage(&_message, &field);
            BOOST_ASSERT(msg);
            bond::Apply(ToProto{ *msg }, value);
        }

        template <typename T>
        typename boost::enable_if<bond::is_bond_type<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            auto msg = _reflection.AddMessage(&_message, &field);
            BOOST_ASSERT(msg);
            bond::Apply(ToProto{ *msg }, value);
        }

        template <typename T>
        typename boost::enable_if<std::is_enum<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.SetEnumValue(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<std::is_enum<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.AddEnumValue(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<bond::is_signed_int<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.SetInt32(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<bond::is_signed_int<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.AddInt32(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.SetUInt32(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<std::is_unsigned<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, T value) const
        {
            _reflection.AddUInt32(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, const google::protobuf::string& value) const
        {
            _reflection.SetString(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<bond::is_string<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            SetValue(field, google::protobuf::string{ string_data(value), string_length(value) });
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, const google::protobuf::string& value) const
        {
            _reflection.AddString(&_message, &field, value);
        }

        template <typename T>
        typename boost::enable_if<bond::is_string<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            AddValue(field, google::protobuf::string{ string_data(value), string_length(value) });
        }

        template <typename T>
        typename boost::enable_if<bond::is_wstring<T> >::type
        SetValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            SetValue(field, boost::locale::conv::utf_to_utf<char>(value));
        }

        template <typename T>
        typename boost::enable_if<bond::is_wstring<T> >::type
        AddValue(const google::protobuf::FieldDescriptor& field, const T& value) const
        {
            AddValue(field, boost::locale::conv::utf_to_utf<char>(value));
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, bool value) const
        {
            _reflection.SetBool(&_message, &field, value);
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, bool value) const
        {
            _reflection.AddBool(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, float value) const
        {
            _reflection.SetFloat(&_message, &field, value);
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, float value) const
        {
            _reflection.AddFloat(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, double value) const
        {
            _reflection.SetDouble(&_message, &field, value);
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, double value) const
        {
            _reflection.AddDouble(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, int64_t value) const
        {
            _reflection.SetInt64(&_message, &field, value);
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, int64_t value) const
        {
            _reflection.AddInt64(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, uint64_t value) const
        {
            _reflection.SetUInt64(&_message, &field, value);
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, uint64_t value) const
        {
            _reflection.AddUInt64(&_message, &field, value);
        }

        void SetValue(const google::protobuf::FieldDescriptor& field, const bond::blob& value) const
        {
            SetValue(field, google::protobuf::string{ value.content(), value.size() });
        }

        void AddValue(const google::protobuf::FieldDescriptor& field, const bond::blob& value) const
        {
            AddValue(field, google::protobuf::string{ value.content(), value.size() });
        }

        google::protobuf::Message& _message;
        const google::protobuf::Reflection& _reflection;
        const google::protobuf::Descriptor& _descriptor;
    };

} // anonymous namespace


BOOST_AUTO_TEST_SUITE(ProtobufReaderTests)

template <typename Proto, typename Bond>
void CheckBinaryFormat(const Bond& bond_struct)
{
    Proto proto_struct;
    bond::Apply(ToProto{ proto_struct }, bond_struct);

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

    unittest::BoxWrongPackingWrongEncoding<std::vector<bond::blob> > box;
    box.value.resize(2, bond::blob{});
    CheckBinaryFormat<unittest::proto::BlobContainer>(box);

    CheckBinaryFormat<
        unittest::proto::StructContainer,
        unittest::BoxWrongPackingWrongEncoding<std::vector<unittest::Integers> > >();

    CheckBinaryFormat<unittest::proto::IntegerMapKeys, unittest::IntegerMapKeys>();
}

BOOST_AUTO_TEST_SUITE_END()

bool init_unit_test()
{
    boost::debug::detect_memory_leaks(false);
    return true;
}
