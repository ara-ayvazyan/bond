#pragma once

#include "unit_test_util.h"
#include "to_proto.h"
#include "protobuf_writer_types.h"
#include "protobuf_writer.pb.h"

#include <google/protobuf/wrappers.pb.h>

#include <bond/protocol/protobuf_binary_reader.h>

#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/map.hpp>


namespace bond
{
    template <> struct
    is_protocol_enabled<ProtobufBinaryReader<InputBuffer> >
        : std::true_type {};

} // namespace bond


inline const char* GetMagicBytes()
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

inline void SetValue(google::protobuf::StringValue& value)
{
    value.set_value("1");
}

inline void SetValue(google::protobuf::BytesValue& value)
{
    value.set_value(GetMagicBytes());
}

inline void SetValue(unittest::proto::BlobMapValue& value)
{
    (*value.mutable_value())[1] = "1";
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
