#include "Render/Shader.hpp"
#include "Core/Logger.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef __platform_web
#include <src/tint/lang/spirv/writer/helpers/generate_bindings.h>
#include <src/tint/lang/spirv/writer/printer/printer.h>
#include <src/tint/lang/spirv/writer/raise/raise.h>

#include <src/tint/lang/core/ir/var.h>
#include <src/tint/lang/core/type/pointer.h>
#include <src/tint/lang/wgsl/ast/module.h>
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

Shader::Expected<Ref<Shader>> Shader::compile(const std::string& filename, const std::vector<std::string>& definitions_list)
{
    Ref<Shader> shader = make_ref<Shader>();
    auto result = shader->compile_internal(filename, definitions_list);
    YEET(result);

#ifdef __has_shader_hot_reload
    shaders.push_back(shader);
#endif

    return shader;
}

Shader::Expected<void> Shader::compile_internal(const std::string& filename, const std::vector<std::string>& definitions_list)
{
    std::ifstream file_stream(filename);

    if (!file_stream.is_open())
    {
        error("shader compilation: {}: File not found", filename);
        return std::unexpected(ErrorKind::FileNotFound);
    }

    std::vector<std::string> definitions = definitions_list;

#ifndef __platform_web
    definitions.push_back("__has_immediate");
#endif

    auto output_result = preprocess(file_stream, definitions);
    if (!output_result.has_value())
    {
        error("shader compilation: {}: Preprocessing failed: {}", filename, (uint8_t)output_result.error());
        return std::unexpected(output_result.error());
    }

    std::string& output = output_result.value();

#ifdef __has_shader_hot_reload
    m_filename = filename;
    m_file_last_write = std::filesystem::last_write_time(std::filesystem::path(filename));
    m_definitions = definitions;
#endif

#ifdef __platform_web
    m_code = output;
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
        error("{}", ir.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::Result<tint::SuccessType> check = tint::core::ir::Validate(ir.Get(), tint::core::ir::Capabilities{
                                                                                   // Allow having vertex & fragment entry points in the same shader, also `TINT_ENABLE_IR_VALIDATION` must be turned of during compilation
                                                                                   // or `tint::spirv::writer::Generate` will fail later.
                                                                                   tint::core::ir::Capability::kAllowMultipleEntryPoints,
                                                                               });

    if (check != tint::Success)
    {
        error("{}", check.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    fill_info(ir.Get());

    tint::spirv::writer::Options gen_options;
    gen_options.bindings = tint::spirv::writer::GenerateBindings(ir.Get());

    check = tint::spirv::writer::CanGenerate(ir.Get(), gen_options);

    if (check != tint::Success)
    {
        error("{}", check.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    tint::Result<tint::spirv::writer::Output> result = tint::spirv::writer::Generate(ir.Get(), gen_options);

    if (result != tint::Success)
    {
        error("{}", result.Failure().reason);
        return std::unexpected(ErrorKind::Compilation);
    }

    m_code = result->spirv;
#endif // __platform_web

    return {};
}

#ifdef __has_shader_hot_reload

void Shader::reload_if_needed()
{
    auto time_point = std::filesystem::last_write_time(std::filesystem::path(m_filename));

    if (time_point <= m_file_last_write)
    {
        return;
    }

    error("Shader {} was modified and will be reloaded...", m_filename);

    Expected<void> result = compile_internal(m_filename, m_definitions);

    if (result.has_value())
    {
        m_was_reloaded = true;
    }
    else
    {
        error("Shader {} failed to compile.", m_filename);
    }
}

std::vector<Ref<Shader>> Shader::shaders;

#endif

#ifndef __platform_web

static TextureDimension convert_dimension(tint::core::type::TextureDimension dim)
{
    switch (dim)
    {
    case tint::core::type::TextureDimension::k1d:
        return TextureDimension::D1D;
    case tint::core::type::TextureDimension::k2d:
        return TextureDimension::D2D;
    case tint::core::type::TextureDimension::k2dArray:
        return TextureDimension::D2DArray;
    case tint::core::type::TextureDimension::k3d:
        return TextureDimension::D3D;
    case tint::core::type::TextureDimension::kCube:
        return TextureDimension::Cube;
    case tint::core::type::TextureDimension::kCubeArray:
        return TextureDimension::CubeArray;
    case tint::core::type::TextureDimension::kNone:
        return {};
    }

    return {};
}

void Shader::fill_info(const tint::core::ir::Module& ir)
{
    std::vector<const tint::core::ir::Function *> stages;

    for (const auto& func : ir.functions)
    {
        if (func->IsEntryPoint())
        {
            stages.push_back(func);
        }
    }

    for (auto *inst : *ir.root_block)
    {
        auto *var = inst->As<tint::core::ir::Var>();
        if (!var)
        {
            continue;
        }

        if (auto bp = var->BindingPoint())
        {
            auto *ptr = var->Result()->Type()->As<tint::core::type::Pointer>();
            auto *ty = ptr->UnwrapPtr();

            const auto& symbol = ir.NameOf(inst);
            const std::string name = symbol.Name();

            Binding binding;

            if (ty->Is<tint::core::type::Texture>())
            {
                const tint::core::type::Texture *texture = ty->As<tint::core::type::Texture>();
                binding = Binding{.kind = BindingKind::Texture, .group = bp->group, .binding = bp->binding, .dimension = convert_dimension(texture->Dim())};
            }
            else
            {
                switch (ptr->AddressSpace())
                {
                case tint::core::AddressSpace::kUniform:
                    binding = Binding{.kind = BindingKind::UniformBuffer, .group = bp->group, .binding = bp->binding, .dimension = {}};
                    break;
                case tint::core::AddressSpace::kStorage:
                    continue;
                case tint::core::AddressSpace::kHandle:
                case tint::core::AddressSpace::kUndefined:
                case tint::core::AddressSpace::kPixelLocal:
                case tint::core::AddressSpace::kPrivate:
                case tint::core::AddressSpace::kImmediate:
                case tint::core::AddressSpace::kIn:
                case tint::core::AddressSpace::kOut:
                case tint::core::AddressSpace::kFunction:
                case tint::core::AddressSpace::kWorkgroup:
                    continue;
                }
            }

            std::optional<ShaderStageKind> stage = stage_of(ir, inst);
            binding.shader_stage = stage.value();

            m_bindings[name] = binding;
        }
    }
}

std::optional<ShaderStageKind> Shader::stage_of(const tint::core::ir::Module& ir, const tint::core::ir::Instruction *inst)
{
    for (const auto& func : ir.functions)
    {
        if (!func->IsEntryPoint())
        {
            continue;
        }

        for (const auto& func_inst : *func->Block())
        {
            if (inst->Result()->HasUsage(func_inst, 0))
            {
                if (func->IsVertex())
                    return ShaderStageKind::Vertex;
                else if (func->IsFragment())
                    return ShaderStageKind::Fragment;
                else if (func->IsCompute())
                    return ShaderStageKind::Compute;
            }
        }
    }

    return std::nullopt;
}

#endif
