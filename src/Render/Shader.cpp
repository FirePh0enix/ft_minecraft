#include "Render/Shader.hpp"

#include <fstream>
#include <iostream>

#ifndef __platform_web
#include <src/tint/lang/spirv/writer/helpers/generate_bindings.h>
#include <src/tint/lang/spirv/writer/printer/printer.h>
#include <src/tint/lang/spirv/writer/raise/raise.h>
#include <tint/tint.h>
#endif

static bool contains(const std::vector<std::string>& definitions, const std::string& definition)
{
    return std::find(definitions.begin(), definitions.end(), definition) != definitions.end();
}

Shader::Expected<std::string> preprocess(std::ifstream& ifs, const std::vector<std::string>& definitions)
{
    std::string line;

    std::string output;
    bool is_in_block = false;
    bool is_skipped = false;

    while (std::getline(ifs, line))
    {
        if (line.starts_with("#ifdef "))
        {
            if (is_in_block)
                return std::unexpected(Shader::ErrorKind::RecursiveDirective);

            std::string definition = line.substr(7);
            is_in_block = true;
            is_skipped = !contains(definitions, definition);
            continue;
        }
        else if (line.starts_with("#ifndef "))
        {
            if (is_in_block)
                return std::unexpected(Shader::ErrorKind::RecursiveDirective);

            std::string definition = line.substr(8);
            is_in_block = true;
            is_skipped = contains(definitions, definition);
            continue;
        }
        else if (line == "#else")
        {
            if (!is_in_block)
                return std::unexpected(Shader::ErrorKind::MissingDirective);

            is_skipped = !is_skipped;
            continue;
        }
        else if (line == "#endif")
        {
            if (!is_in_block)
                return std::unexpected(Shader::ErrorKind::MissingDirective);

            is_skipped = false;
            is_in_block = false;
            continue;
        }

        if (!is_skipped)
        {
            output += line + "\n";
        }
    }

    return output;
}

Shader::Expected<Ref<Shader>> Shader::compile(const std::string& filename, const std::initializer_list<std::string>& definitions_list)
{
    std::ifstream file_stream(filename);

    if (!file_stream.is_open())
    {
        std::println(stderr, "shader compilation: {}: File not found", filename);
        return std::unexpected(ErrorKind::FileNotFound);
    }

    std::vector<std::string> definitions = definitions_list;

#ifndef __platform_web
    definitions.push_back("__has_immediate");
#endif

    auto output_result = preprocess(file_stream, definitions);
    if (!output_result.has_value())
    {
        std::println(stderr, "shader compilation: {}: Preprocessing failed: {}", filename, (uint8_t)output_result.error());
        return std::unexpected(output_result.error());
    }

    std::string& output = output_result.value();
    Ref<Shader> shader = make_ref<Shader>();

#ifdef __platform_web
    shader->m_code = output;
#else
    tint::Source::File file(filename, output);

    tint::wgsl::reader::Options parse_options;

    // Allow the use of experimental `immediate` address space which will map to push constants on vulkan.
    parse_options.allowed_features.extensions.insert(tint::wgsl::Extension::kChromiumExperimentalImmediate);

    tint::Program program = tint::wgsl::reader::Parse(&file, parse_options);

    if (program.Diagnostics().ContainsErrors())
    {
        std::cerr << program.Diagnostics() << "\n";
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::Result<tint::core::ir::Module> ir = tint::wgsl::reader::ProgramToLoweredIR(program);

    if (ir != tint::Success)
    {
        std::println(stderr, "{}", ir.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::Result<tint::SuccessType> check = tint::core::ir::Validate(ir.Get(), tint::core::ir::Capabilities{
                                                                                   // Allow having vertex & fragment entry points in the same shader, also `TINT_ENABLE_IR_VALIDATION` must be turned of during compilation
                                                                                   // or `tint::spirv::writer::Generate` will fail later.
                                                                                   tint::core::ir::Capability::kAllowMultipleEntryPoints,
                                                                               });

    if (check != tint::Success)
    {
        std::println(stderr, "{}", check.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::spirv::writer::Options gen_options;
    gen_options.bindings = tint::spirv::writer::GenerateBindings(ir.Get());

    check = tint::spirv::writer::CanGenerate(ir.Get(), gen_options);

    if (check != tint::Success)
    {
        std::println(stderr, "{}", check.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::Result<tint::spirv::writer::Output> result = tint::spirv::writer::Generate(ir.Get(), gen_options);

    if (result != tint::Success)
    {
        std::println(stderr, "{}", result.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    shader->m_code = result->spirv;
#endif // __platform_web

    return shader;
}
