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
    class FieldGenerator
    {
    public:
        explicit FieldGenerator(google::protobuf::io::Printer& printer)
            : _printer{ printer }
        {}

        void Generate(const google::protobuf::FieldDescriptor& field)
        {
            if (auto packing = GetFieldPacking(field))
            {
                _printer.Print("[ProtoPack(\"$packing$\")]\n",
                    "packing", packing);
            }

            if (auto encoding = GetFieldEncoding(field))
            {
                _printer.Print("[ProtoEncode(\"$encoding$\")]\n",
                    "encoding", encoding);
            }

            _printer.Print("$required$$id$: $type$ $name$;\n",
                "required", field.label() == google::protobuf::FieldDescriptor::LABEL_REQUIRED ? "required" : "",
                "id", std::to_string(field.number()),
                "type", GetFieldTypeName(field),
                "name", field.name());
        }

    private:
        static const char* GetFieldPacking(const google::protobuf::FieldDescriptor& field)
        {
            return field.is_packable() && !field.is_packed() ? "False" : nullptr;
        }

        static const char* GetFieldEncoding(const google::protobuf::FieldDescriptor& field)
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

        static google::protobuf::string GetFieldTypeName(const google::protobuf::FieldDescriptor& field)
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

            StructGenerator struct_generator{ printer };

            for (int i = 0; i < file.message_type_count(); ++i)
            {
                struct_generator.Generate(*file.message_type(i));
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
