#pragma warning(disable:4127)

#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>

#include <google/protobuf/stubs/scoped_ptr.h>
#include <google/protobuf/stubs/strutil.h>


namespace bond
{
namespace detail
{
namespace proto
{
    class IdlGenerator : public google::protobuf::compiler::CodeGenerator
    {
    public:
        bool Generate(const google::protobuf::FileDescriptor* file,
            const google::protobuf::string& /*parameter*/,
            google::protobuf::compiler::GeneratorContext* generator_context,
            google::protobuf::string* /*error*/) const override
        {
            auto basename = google::protobuf::StripSuffixString(file->name(), ".proto");

            google::protobuf::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> output{
                generator_context->Open(basename + ".bond") };

            google::protobuf::io::Printer printer{ output.get(), '$' };

            printer.Print("namespace $namespace$\n",
                "namespace", file->package());

            for (int i = 0; i < file->message_type_count(); ++i)
            {
                auto msg = file->message_type(i);

                printer.Print("\nstruct $name$\n{\n",
                    "name", msg->name());
                printer.Indent();

                for (int j = 0; j < msg->field_count(); ++j)
                {
                    auto field = msg->field(j);

                    printer.Print("$id$: $type$ $name$;\n",
                        "id", std::to_string(field->number()),
                        "type", field->cpp_type_name(),
                        "name", field->name());
                }

                printer.Outdent();
                printer.Print("}\n");
            }

            return true;
        }

    private:
    };

}}} // namespace bond::detail::proto


int main(int argc, char* argv[])
{
    bond::detail::proto::IdlGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
