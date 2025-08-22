#include "Render/Shader.hpp"
#include "Core/Assert.hpp"
#include "Core/Filesystem.hpp"
#include "Render/ShaderMap.hpp"

#include <iostream>

// TODO: add shader variants back.

static ShaderStageFlagBits convert_stage(SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_VERTEX:
        return ShaderStageFlagBits::Vertex;
    case SLANG_STAGE_FRAGMENT:
        return ShaderStageFlagBits::Fragment;
    case SLANG_STAGE_COMPUTE:
        return ShaderStageFlagBits::Compute;
    default:
        ASSERT_V(false, "{}", (uint32_t)stage);
    }

    return {};
}

static ShaderStageFlagBits stage_from_string(const std::string_view stage)
{
    if (stage == "vertex")
        return ShaderStageFlagBits::Vertex;
    else if (stage == "fragment")
        return ShaderStageFlagBits::Fragment;
    else if (stage == "compute")
        return ShaderStageFlagBits::Compute;
    else
        ASSERT_V(false, "{}", stage);
    return {};
}

static TextureDimension convert_dimension(SlangResourceShape shape)
{
    switch (shape)
    {
    case SLANG_TEXTURE_1D:
        return TextureDimension::D1D;
    case SLANG_TEXTURE_2D:
        return TextureDimension::D2D;
    case SLANG_TEXTURE_2D_ARRAY:
        return TextureDimension::D2DArray;
    case SLANG_TEXTURE_3D:
        return TextureDimension::D3D;
    case SLANG_TEXTURE_CUBE:
        return TextureDimension::Cube;
    case SLANG_TEXTURE_CUBE_ARRAY:
        return TextureDimension::CubeArray;
    default:
        assert(0);
    }
}

Result<Ref<Shader>> Shader::load(const std::filesystem::path& path)
{
    Ref<Shader> shader = make_ref<Shader>();
    Result<std::string> source_code_result = Filesystem::read_file_to_string(path);
    YEET(source_code_result);

    shader->m_source_code = source_code_result.value();

    // Re-use a global session to reduce load times.
    if (!s_global_session)
        slang::createGlobalSession(s_global_session.writeRef());

    slang::SessionDesc session_desc{};
    slang::TargetDesc target_desc{};

    std::vector<slang::CompilerOptionEntry> options;

    // NOTE: If we use slang's default of row major matrices we will need to transpose matrices before passing them to shaders.
    // options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::MatrixLayoutColumn, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr)));

#ifndef __platform_web
    // On linux we use Vulkan so we need SPIR-V.
    target_desc.format = SLANG_SPIRV;
    target_desc.profile = s_global_session->findProfile("spirv_1_5");

    options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::EmitSpirvDirectly, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr)));
#else
    target_desc.format = SLANG_WGSL;
#endif

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    session_desc.compilerOptionEntries = options.data();
    session_desc.compilerOptionEntryCount = options.size();

    std::vector<slang::PreprocessorMacroDesc> preprocessor_macro_desc{};
    session_desc.preprocessorMacros = preprocessor_macro_desc.data();
    session_desc.preprocessorMacroCount = (SlangInt)preprocessor_macro_desc.size();

    // TODO: Add type specialization

    SlangResult result = s_global_session->createSession(session_desc, shader->m_session.writeRef());
    if (SLANG_FAILED(result))
        return Error(ErrorKind::ShaderCompilationFailed);

    Slang::ComPtr<slang::IBlob> diagnostics_blob;
    std::filesystem::path module_name = path.filename().replace_extension();

    shader->m_module = shader->m_session->loadModuleFromSourceString(module_name.string().c_str(), path.string().c_str(), shader->m_source_code.data(), diagnostics_blob.writeRef());
    if (!shader->m_module)
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    std::vector<std::string> entry_points;

    slang::DeclReflection *reflection = shader->m_module->getModuleReflection();
    for (const auto& child : reflection->getChildren())
    {
        if (child->getKind() == slang::DeclReflection::Kind::Func)
        {
            slang::FunctionReflection *func = child->asFunction();
            slang::Attribute *attrib = func->findAttributeByName(s_global_session, "shader");

            if (attrib)
            {
                size_t name_size = 0;
                const char *name_s = attrib->getArgumentValueString(0, &name_size);
                std::string_view s(name_s);

                shader->m_stage_mask |= stage_from_string(s);
                entry_points.push_back(func->getName());
            }
        }
    }

    // Compile the program, link and receive the SPIR-V bytecode.
    std::vector<slang::IComponentType *> component_types{shader->m_module};

    for (const auto& entry_point : entry_points)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point_comp;
        shader->m_module->findEntryPointByName(entry_point.c_str(), entry_point_comp.writeRef());
        component_types.push_back(entry_point_comp);
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    result = shader->m_session->createCompositeComponentType(component_types.data(), (SlangInt)component_types.size(), composed_program.writeRef(), diagnostics_blob.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    Slang::ComPtr<slang::IComponentType> linked_program;
    result = composed_program->link(linked_program.writeRef(), diagnostics_blob.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    // Extract reflection informations.
    dump_glsl(path);

    slang::ProgramLayout *program_layout = linked_program->getLayout();
    slang::VariableLayoutReflection *gvar_layout = program_layout->getGlobalParamsVarLayout();
    slang::TypeLayoutReflection *type_layout = gvar_layout->getTypeLayout();

    println("{} <> {}", path, type_layout->getFieldCount());

    for (size_t i = 0; i < type_layout->getFieldCount(); i++)
    {
        slang::VariableLayoutReflection *var = type_layout->getFieldByIndex(i);
        std::string name = var->getName();
        uint32_t group = var->getBindingSpace();
        uint32_t binding = var->getBindingIndex();
        slang::BindingType binding_type = var->getTypeLayout()->getBindingRangeType(0);

        // FIXME: `getStage()` returns always 0 for some reason, so for now in the Vulkan code uniforms use VK_SHADER_STAGE_ALL.
        // ShaderStageFlagBits stage = convert_stage(var->getStage());

        if (binding_type == slang::BindingType::PushConstant)
            continue;

        switch (var->getType()->getKind())
        {
        case slang::TypeReflection::Kind::ConstantBuffer:
            shader->m_bindings[name] = Binding(BindingKind::UniformBuffer, ShaderStageFlags(), group, binding);
            break;
        case slang::TypeReflection::Kind::Resource:
        {
            bool is_combined = (var->getType()->getResourceShape() & SLANG_TEXTURE_COMBINED_FLAG) == SLANG_TEXTURE_COMBINED_FLAG;
            ASSERT(is_combined, "Only combined image samplers are supported");
            TextureDimension dimension = convert_dimension((SlangResourceShape)(var->getType()->getResourceShape() & ~SLANG_TEXTURE_COMBINED_FLAG));
            shader->m_bindings[name] = Binding(BindingKind::Texture, ShaderStageFlags(), group, binding, dimension);
        }
        break;
        default:
            warn("unsupported shader variable `{}` for `{}`", (uint32_t)var->getType()->getKind(), name);
            break;
        }
    }

    // Get all entry points in one binary blob on Vulkan.
    Slang::ComPtr<slang::IBlob> binary_code;
    result = linked_program->getTargetCode(0, binary_code.writeRef(), diagnostics_blob.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

#ifndef __platform_web
    shader->m_binary_code.resize(binary_code->getBufferSize() / 4);
    std::memcpy(shader->m_binary_code.data(), binary_code->getBufferPointer(), binary_code->getBufferSize());
#else
    shader->m_binary_code.append(binary_code->getBufferPointer(), binary_code->getBufferSize());
#endif

    return shader;
}

void Shader::dump_glsl(const std::filesystem::path& path)
{
    Ref<Shader> shader = make_ref<Shader>();
    Result<std::string> source_code_result = Filesystem::read_file_to_string(path);

    shader->m_source_code = source_code_result.value();

    // Re-use a global session to reduce load times.
    if (!s_global_session)
        slang::createGlobalSession(s_global_session.writeRef());

    slang::SessionDesc session_desc{};
    slang::TargetDesc target_desc{};

    std::vector<slang::CompilerOptionEntry> options;

#ifndef __platform_web
    // On linux we use Vulkan so we need SPIR-V.
    target_desc.format = SLANG_GLSL;
    target_desc.profile = s_global_session->findProfile("vulkan");

    options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::EmitSpirvDirectly, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr)));
#else
    target_desc.format = SLANG_WGSL;
#endif

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    session_desc.compilerOptionEntries = options.data();
    session_desc.compilerOptionEntryCount = options.size();

    std::vector<slang::PreprocessorMacroDesc> preprocessor_macro_desc{};
    session_desc.preprocessorMacros = preprocessor_macro_desc.data();
    session_desc.preprocessorMacroCount = (SlangInt)preprocessor_macro_desc.size();

    // TODO: Add type specialization

    SlangResult result = s_global_session->createSession(session_desc, shader->m_session.writeRef());
    (void)result;

    Slang::ComPtr<slang::IBlob> diagnostics_blob;
    std::filesystem::path module_name = path.filename().replace_extension();

    shader->m_module = shader->m_session->loadModuleFromSourceString(module_name.string().c_str(), path.string().c_str(), shader->m_source_code.data(), diagnostics_blob.writeRef());

    std::vector<std::string> entry_points;

    slang::DeclReflection *reflection = shader->m_module->getModuleReflection();
    for (const auto& child : reflection->getChildren())
    {
        if (child->getKind() == slang::DeclReflection::Kind::Func)
        {
            slang::FunctionReflection *func = child->asFunction();
            slang::Attribute *attrib = func->findAttributeByName(s_global_session, "shader");

            if (attrib)
                entry_points.push_back(func->getName());
        }
    }

    // Compile the program, link and receive the SPIR-V bytecode.
    std::vector<slang::IComponentType *> component_types{shader->m_module};

    for (const auto& entry_point : entry_points)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point_comp;
        shader->m_module->findEntryPointByName(entry_point.c_str(), entry_point_comp.writeRef());
        component_types.push_back(entry_point_comp);
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    result = shader->m_session->createCompositeComponentType(component_types.data(), (SlangInt)component_types.size(), composed_program.writeRef(), diagnostics_blob.writeRef());

    Slang::ComPtr<slang::IComponentType> linked_program;
    result = composed_program->link(linked_program.writeRef(), diagnostics_blob.writeRef());

    // Get all entry points in one binary blob on Vulkan.
    Slang::ComPtr<slang::IBlob> binary_code;
    result = linked_program->getTargetCode(0, binary_code.writeRef(), diagnostics_blob.writeRef());

    println("{}", (char *)binary_code->getBufferPointer());
}
