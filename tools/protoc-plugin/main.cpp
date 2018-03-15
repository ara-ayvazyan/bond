#pragma warning(disable:4127 4146 4800)

#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>

#include <google/protobuf/stubs/strutil.h>

#include <memory>


namespace bond
{
namespace detail
{
namespace proto
{
    class EnumGenerator
    {
    public:
        explicit EnumGenerator(google::protobuf::io::Printer& printer)
            : _printer{ printer }
        {}

        void Generate(const google::protobuf::EnumDescriptor& en)
        {
            _printer.Print("\nenum $name$\n{\n",
                "name", en.name());
            _printer.Indent();

            for (int i = 0; i < en.value_count(); ++i)
            {
                if (i != 0)
                {
                    _printer.Print(",\n");
                }

                auto value = en.value(i);

                _printer.Print("$name$ = $value$",
                    "name", value->name(),
                    "value", std::to_string(value->number()));
            }

            _printer.Outdent();
            _printer.Print("\n}\n");
        }

    private:
        google::protobuf::io::Printer& _printer;
    };


    class FieldGenerator
    {
    public:
        explicit FieldGenerator(google::protobuf::io::Printer& printer)
            : _printer{ printer }
        {}

        void Generate(const google::protobuf::FieldDescriptor& field)
        {
            if (auto packing = GetPacking(field))
            {
                _printer.Print("[ProtoPack(\"$packing$\")]\n",
                    "packing", packing);
            }

            if (auto encoding = GetEncoding(field))
            {
                _printer.Print("[ProtoEncode(\"$encoding$\")]\n",
                    "encoding", encoding);
            }

            auto default_value = GetDefaultValue(field);

            _printer.Print("$id$:$required$ $type$ $name$$default$;\n",
                "required", field.label() == google::protobuf::FieldDescriptor::LABEL_REQUIRED ? " required" : "",
                "id", std::to_string(field.number()),
                "type", GetTypeName(field),
                "name", field.name(),
                "default", default_value.empty() ? "" : (" = " + default_value));
        }

    private:
        static const char* GetPacking(const google::protobuf::FieldDescriptor& field)
        {
            return field.is_packable() && !field.is_packed() ? "False" : nullptr;
        }

        static const char* GetEncoding(const google::protobuf::FieldDescriptor& field)
        {
            switch (field.type())
            {
            case google::protobuf::FieldDescriptor::TYPE_FIXED64:
            case google::protobuf::FieldDescriptor::TYPE_FIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED32:
            case google::protobuf::FieldDescriptor::TYPE_SFIXED64:
                return "Fixed";

            case google::protobuf::FieldDescriptor::TYPE_SINT32:
            case google::protobuf::FieldDescriptor::TYPE_SINT64:
                return "ZigZag";
            }

            return nullptr;
        }

        static google::protobuf::string GetTypeName(const google::protobuf::FieldDescriptor& field)
        {
            if (field.is_map())
            {
                auto entry = field.message_type();
                return "map<"
                    + GetBasicTypeName(*entry->FindFieldByName("key"))
                    + ", "
                    + GetBasicTypeName(*entry->FindFieldByName("value"))
                    + ">";
            }

            auto name = GetBasicTypeName(field);

            return field.is_repeated() ? ("vector<" + name + ">") : name;
        }

        static google::protobuf::string GetBasicTypeName(const google::protobuf::FieldDescriptor& field)
        {
            switch (field.cpp_type())
            {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                return "int32";

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                return "int64";

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                return "uint32";

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                return "uint64";

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                return "double";

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                return "float";

            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                return "bool";

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                return field.enum_type()->name();

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                return field.type() == google::protobuf::FieldDescriptor::TYPE_BYTES ? "blob" : "string";

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                return field.options().lazy()
                    ? ("bonded<" + field.message_type()->name() + ">")
                    : field.message_type()->name();
            }

            assert(false);
            return nullptr;
        }

        static google::protobuf::string GetDefaultValue(const google::protobuf::FieldDescriptor& field)
        {
            if (!field.has_default_value())
            {
                return "";
            }

            switch (field.cpp_type())
            {
            case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                return std::to_string(field.default_value_int32());

            case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                return std::to_string(field.default_value_int64());

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                return std::to_string(field.default_value_uint32());

            case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                return std::to_string(field.default_value_uint64());

            case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                return std::to_string(field.default_value_double());

            case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                return std::to_string(field.default_value_float());

            case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                return field.default_value_bool() ? "true" : "false";

            case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                return field.enum_type()->value(field.default_value_enum()->index())->name();

            case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                return "\"" + field.default_value_string() + "\""; // TODO: Replace " with \"

            case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                return "";
            }

            assert(false);
            return "";
        }

        google::protobuf::io::Printer& _printer;
    };


    class StructGenerator
    {
    public:
        explicit StructGenerator(google::protobuf::io::Printer& printer)
            : _printer{ printer }
        {}

        void Generate(const google::protobuf::Descriptor& msg)
        {
            _printer.Print("\nstruct $name$\n{\n",
                "name", msg.name());
            _printer.Indent();

            FieldGenerator field_generator{ _printer };

            for (int i = 0; i < msg.field_count(); ++i)
            {
                if (i != 0)
                {
                    _printer.Print("\n");
                }

                field_generator.Generate(*msg.field(i));
            }

            _printer.Outdent();
            _printer.Print("}\n");
        }

    private:
        google::protobuf::io::Printer& _printer;
    };


    class FileGenerator
    {
    public:
        void Generate(const google::protobuf::FileDescriptor& file, google::protobuf::compiler::GeneratorContext& context)
        {
            auto basename = google::protobuf::StripSuffixString(file.name(), ".proto");

            std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output{ context.Open(basename + ".bond") };
            google::protobuf::io::Printer printer{ output.get(), '$' };

            printer.Print("namespace $namespace$\n",
                "namespace", file.package());

            {
                EnumGenerator enum_generator{ printer };

                for (int i = 0; i < file.enum_type_count(); ++i)
                {
                    enum_generator.Generate(*file.enum_type(i));
                }
            }
            {
                StructGenerator struct_generator{ printer };

                for (int i = 0; i < file.message_type_count(); ++i)
                {
                    struct_generator.Generate(*file.message_type(i));
                }
            }
        }
    };


    class Generator : public google::protobuf::compiler::CodeGenerator
    {
    public:
        bool Generate(const google::protobuf::FileDescriptor* file,
            const google::protobuf::string& /*parameter*/,
            google::protobuf::compiler::GeneratorContext* generator_context,
            google::protobuf::string* error) const override
        {
            try
            {
                FileGenerator file_generator;
                file_generator.Generate(*file, *generator_context);
                return true;
            }
            catch (const std::exception& e)
            {
                *error = e.what();
                return false;
            }
            catch (...)
            {
                *error = "Unknown error";
                return false;
            }
        }
    };

}}} // namespace bond::detail::proto


int main(int argc, char* argv[])
{
    bond::detail::proto::Generator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
