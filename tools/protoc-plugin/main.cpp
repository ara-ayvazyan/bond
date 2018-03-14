#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/code_generator.h>


namespace bond
{
namespace detail
{
namespace proto
{
    class IdlGenerator : public google::protobuf::compiler::CodeGenerator
    {
    public:
        bool Generate(const google::protobuf::FileDescriptor* /*file*/,
            const google::protobuf::string& /*parameter*/,
            google::protobuf::compiler::GeneratorContext* /*generator_context*/,
            google::protobuf::string* error) const override
        {
            *error = "Not implemented";
            return false;
        }

    private:
    };

}}} // namespace bond::detail::proto


int main(int argc, char* argv[])
{
    bond::detail::proto::IdlGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
