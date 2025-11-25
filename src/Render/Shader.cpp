#include "Render/Shader.hpp"
#include "Core/Assert.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Print.hpp"

#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>

#include <regex>

// TODO: add shader variants back.

static Slang::ComPtr<slang::IGlobalSession> s_global_session;

struct MemoryBlob : public ISlangBlob
{
    SLANG_REF_OBJECT_IUNKNOWN_ALL;

    MemoryBlob(const std::vector<char>& buffer)
        : m_buffer(buffer)
    {
    }

    virtual SLANG_NO_THROW void const *SLANG_MCALL getBufferPointer() override
    {
        return (void *)m_buffer.data();
    }

    virtual SLANG_NO_THROW size_t SLANG_MCALL getBufferSize() override
    {
        return m_buffer.size();
    }

    ISlangUnknown *getInterface(const SlangUUID& uuid)
    {
        (void)uuid;
        return nullptr;
    }

    uint32_t addReference()
    {
        return ++m_refCount;
    }

    uint32_t releaseReference()
    {
        return --m_refCount;
    }

private:
    uint32_t m_refCount = 0;
    std::vector<char> m_buffer;
};

struct UnimplementedFileSystem : public ISlangFileSystem
{
    SLANG_REF_OBJECT_IUNKNOWN_ALL;

    virtual SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(char const *path, ISlangBlob **out_blob) override
    {
        (void)path;
        (void)out_blob;

        Result<File> file = Filesystem::open_file(path);
        if (file.has_error())
            return SLANG_E_NOT_FOUND;

        Result<std::vector<char>> result = file->read_to_buffer();
        if (result.has_error())
            return SLANG_E_NOT_FOUND;
        *out_blob = new MemoryBlob(result.value());

        return SLANG_OK;
    }

    ISlangUnknown *getInterface(const SlangUUID& uuid)
    {
        (void)uuid;
        return nullptr;
    }

    uint32_t addReference()
    {
        return ++m_refCount;
    }

    uint32_t releaseReference()
    {
        return --m_refCount;
    }

    virtual SLANG_NO_THROW void *SLANG_MCALL castAs(const SlangUUID& guid) override
    {
        (void)guid;
        return nullptr;
    }

    uint32_t m_refCount = 0;
};

struct EntryPoint
{
    ShaderStageFlagBits stage = ShaderStageFlagBits::Vertex;
    Slang::ComPtr<slang::IEntryPoint> entry_point;
    Slang::ComPtr<slang::IMetadata> metadata;
};

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

Shader::~Shader()
{
    if (m_code)
        free(m_code);
}

Result<Ref<Shader>> Shader::load(const std::filesystem::path& path, bool compute_shader)
{
    if (path.extension() == ".glsl" || path.extension() == ".comp" || path.extension() == ".vert" || path.extension() == ".frag")
        return load_glsl_shader(path);
    return load_slang_shader(path, compute_shader);
}

Result<Ref<Shader>> Shader::load_glsl_shader(const std::filesystem::path& path)
{
    Ref<Shader> shader = newobj(Shader);
    Result<std::string> source_code_result = Filesystem::open_file(path).value().read_to_string();
    YEET(source_code_result);

    shader->m_source_code = source_code_result.value();
    shader->m_path = path.string();
    shader->m_kind = ShaderKind::GLSL;

    shader->m_bindings["info"] = Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Compute, 0, 0, BindingBuffer(16, false));
    shader->m_bindings["simplexState"] = Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Compute, 0, 1, BindingBuffer(256, false));
    shader->m_bindings["blocks"] = Binding(BindingKind::StorageBuffer, ShaderStageFlagBits::Compute, 0, 2, BindingBuffer(4, true));

    shader->m_stage_mask = ShaderStageFlagBits::Compute;
    shader->m_entry_point_names[ShaderStageFlagBits::Compute] = "main";

    return shader;
}

static void remove_substrings(std::string& s, const std::string& p)
{
    std::string::size_type n = p.length();
    for (std::string::size_type i = s.find(p);
         i != std::string::npos;
         i = s.find(p))
        s.erase(i, n);
}

Result<Ref<Shader>> Shader::load_slang_shader(const std::filesystem::path& path, bool compute_shader)
{
    Ref<Shader> shader = newobj(Shader);
    Result<std::string> source_code_result = Filesystem::open_file(path).value().read_to_string();
    YEET(source_code_result);

    shader->m_source_code = source_code_result.value();
    shader->m_path = path.string();

    // Re-use a global session to reduce load times.
    if (!s_global_session)
        slang::createGlobalSession(s_global_session.writeRef());

    slang::SessionDesc session_desc{};
    slang::TargetDesc target_desc{};

    Slang::ComPtr<ISlangFileSystem> filesystem = Slang::ComPtr<ISlangFileSystem>(new UnimplementedFileSystem());
    session_desc.fileSystem = filesystem.get();

    std::vector<slang::CompilerOptionEntry> options;

    // NOTE: If we use slang's default of row major matrices we will need to transpose matrices before passing them to shaders.
    options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::MatrixLayoutColumn, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1)));
    // options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::Optimization, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1)));

#ifndef __platform_web
    if (!compute_shader)
    {
        target_desc.format = SLANG_SPIRV;
        target_desc.profile = s_global_session->findProfile("spirv_1_3");
        options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::EmitSpirvDirectly, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1)));

        shader->m_kind = ShaderKind::SPIRV;
    }
    else
    {
        target_desc.format = SLANG_GLSL;
        shader->m_kind = ShaderKind::GLSL;
    }

    // target_desc.format = SLANG_SPIRV;
    // target_desc.profile = s_global_session->findProfile("spirv_1_3");
    // options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::EmitSpirvMethod, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, SlangEmitSpirvMethod::SLANG_EMIT_SPIRV_VIA_GLSL)));

    // shader->m_kind = ShaderKind::SPIRV;
#else
    target_desc.format = SLANG_WGSL;

    shader->m_kind = ShaderKind::WGSL;
#endif

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    session_desc.compilerOptionEntries = options.data();
    session_desc.compilerOptionEntryCount = options.size();

    std::vector<slang::PreprocessorMacroDesc> preprocessor_macro_desc{};
    session_desc.preprocessorMacros = preprocessor_macro_desc.data();
    session_desc.preprocessorMacroCount = (SlangInt)preprocessor_macro_desc.size();

    // TODO: Add type specialization
    Slang::ComPtr<slang::IBlob> diagnostics_blob;

    Slang::ComPtr<slang::ISession> session;
    SlangResult result = s_global_session->createSession(session_desc, session.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    std::filesystem::path module_name = path.filename().replace_extension();

    Slang::ComPtr<slang::IModule> module = Slang::ComPtr(session->loadModuleFromSourceString(module_name.string().c_str(), path.string().c_str(), shader->m_source_code.data(), diagnostics_blob.writeRef()));
    if (!module)
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    std::vector<EntryPoint> entry_points;
    std::vector<slang::IComponentType *> component_types{module};

    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); i++)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point_comp;
        module->getDefinedEntryPoint(i, entry_point_comp.writeRef());

        // NOTE: This works but dont seems the best way to do it.
        slang::Attribute *attrib = entry_point_comp->getFunctionReflection()->findAttributeByName(s_global_session, "shader");
        size_t name_size = 0;
        std::string_view name(attrib->getArgumentValueString(0, &name_size));

        EntryPoint entry_point;
        entry_point.entry_point = entry_point_comp;
        entry_point.stage = stage_from_string(name);

        entry_points.push_back(entry_point);
        component_types.push_back(entry_point_comp);

        shader->m_entry_point_names[entry_point.stage] = entry_point_comp->getFunctionReflection()->getName();
    }

    // Compile the program, link and receive the SPIR-V bytecode.
    Slang::ComPtr<slang::IComponentType> composed_program;
    result = session->createCompositeComponentType(component_types.data(), (SlangInt)component_types.size(), composed_program.writeRef(), diagnostics_blob.writeRef());
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
    slang::ProgramLayout *program_layout = linked_program->getLayout();

    for (size_t i = 0; i < entry_points.size(); i++)
    {
        result = linked_program->getEntryPointMetadata((SlangInt)i, 0, entry_points[i].metadata.writeRef(), diagnostics_blob.writeRef());
        if (SLANG_FAILED(result))
        {
            println("{}", (char *)diagnostics_blob->getBufferPointer());
            return Error(ErrorKind::ShaderCompilationFailed);
        }
    }

    println("> shader `{}`", path);
    // println(">>> {}", program_layout->getParameterByIndex(0)->getName());

    for (size_t i = 0; i < program_layout->getParameterCount(); i++)
    {
        slang::VariableLayoutReflection *var = program_layout->getParameterByIndex(i);
        std::string name = var->getName();
        uint32_t group = var->getBindingSpace();
        uint32_t binding = var->getBindingIndex();
        slang::BindingType binding_type = var->getTypeLayout()->getBindingRangeType(0);

        // Compute which stages use this variable.
        ShaderStageFlags stages;
        for (size_t i = 0; i < entry_points.size(); i++)
        {
            bool used = false;
            entry_points[i].metadata->isParameterLocationUsed(SlangParameterCategory(var->getCategory()), group, binding, used);
            if (used)
                stages |= entry_points[i].stage;
        }

        if (binding_type == slang::BindingType::Unknown)
        {
            continue;
        }
        else if (binding_type == slang::BindingType::PushConstant)
        {
            println(">> push_constants, size = {}", var->getTypeLayout()->getElementTypeLayout()->getSize());
            shader->m_push_constants.push_back(PushConstantRange(stages, var->getTypeLayout()->getElementTypeLayout()->getSize()));
            continue;
        }

        switch (var->getType()->getKind())
        {
        case slang::TypeReflection::Kind::ConstantBuffer:
        {
            println(">> constbuffer at {}:{}", group, binding);
            const size_t byte_size = var->getTypeLayout()->getElementTypeLayout()->getSize();
            shader->m_bindings[name] = Binding(BindingKind::UniformBuffer, stages, group, binding, BindingBuffer(byte_size, false));
        }
        break;
        case slang::TypeReflection::Kind::Resource:
        {
            SlangResourceShape shape = var->getType()->getResourceShape();
            // SlangResourceAccess access = var->getType()->getResourceAccess();

            if (shape == SLANG_STRUCTURED_BUFFER)
            {
                println(">> buffer at {}:{}", group, binding);
                const size_t byte_size = var->getTypeLayout()->getElementTypeLayout()->getSize();
                shader->m_bindings[name] = Binding(BindingKind::StorageBuffer, stages, group, binding, BindingBuffer(byte_size, true));
            }
            else if ((shape & SLANG_TEXTURE_COMBINED_FLAG) == SLANG_TEXTURE_COMBINED_FLAG || (shape & SLANG_TEXTURE_1D) == SLANG_TEXTURE_1D || (shape & SLANG_TEXTURE_2D) == SLANG_TEXTURE_2D || (shape & SLANG_TEXTURE_3D) == SLANG_TEXTURE_3D || (shape & SLANG_TEXTURE_CUBE) == SLANG_TEXTURE_CUBE)
            {
                // TODO: This does not make any difference between combined images and not, meaning when manually specifying a sampler
                //       it must be right after the texture (location + 1).
                SlangResourceShape shape_without_flags = (SlangResourceShape)(shape & ~SLANG_TEXTURE_COMBINED_FLAG);
                TextureDimension dimension = convert_dimension(shape_without_flags);
                shader->m_bindings[name] = Binding(BindingKind::Texture, stages, group, binding, dimension);

                println(">> texture at {}:{}", group, binding);
            }
            else
            {
                ASSERT_V(false, "Unsupported resource `{}`", (SlangInt)shape);
            }
        }
        break;
        case slang::TypeReflection::Kind::SamplerState:
            warn("TODO: manual sampler support could be better");
            break;
        default:
            warn("unsupported shader variable `{}` for `{}`", (uint32_t)var->getType()->getKind(), name);
            break;
        }
    }

    // Get all entry points in one blob.
    Slang::ComPtr<slang::IBlob> output_code;
    result = linked_program->getTargetCode(0, output_code.writeRef(), diagnostics_blob.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

#if defined(__platform_linux) || defined(__platform_windows) || defined(__platform_macos)
    // FIXME: On MacOS `aligned_alloc(sizeof(uint32_t), output_code->getBufferSize())` returns nullptr
    // shader->m_code = (char *)aligned_alloc(sizeof(uint32_t), output_code->getBufferSize());

    if (!compute_shader)
    {
        shader->m_code = (char *)malloc(output_code->getBufferSize());
        shader->m_size = output_code->getBufferSize();
        std::memcpy(shader->m_code, output_code->getBufferPointer(), output_code->getBufferSize());
    }
    else
    {
        std::string s;
        s.append((char *)output_code->getBufferPointer(), output_code->getBufferSize());

        remove_substrings(s, "layout(row_major) uniform;");
        remove_substrings(s, "layout(row_major) buffer;");

        shader->m_code = strdup(s.c_str());
        shader->m_size = s.size();
    }

    // println("m_size = {}, code_u32_size = {}", output_code->getBufferSize(), shader->get_code_u32().size() * sizeof(uint32_t));

    // println("{}", StringView(shader->m_code, shader->m_size));

    // std::filesystem::path path2 = path;
    // path2.replace_extension(".slang.spv");

    // std::ofstream os(Filesystem::resolve_absolute(path2));
    // os.write(shader->m_code, (std::streamsize)shader->m_size);

    // shader->dump_glsl();

    // std::string source;
    // source.append((char *)output_code->getBufferPointer(), output_code->getBufferSize());

    // println("{}", source);

    // shader->m_code = (char *)aligned_alloc(sizeof(uint32_t), output_code->getBufferSize());
    // std::memcpy(shader->m_code, output_code->getBufferPointer(), output_code->getBufferSize());
#else
    shader->m_code = (char *)malloc(sizeof(char) * (output_code->getBufferSize() + 1));
    shader->m_size = output_code->getBufferSize();
    std::memcpy(shader->m_code, output_code->getBufferPointer(), output_code->getBufferSize());
    shader->m_code[shader->m_size] = '\0';

    println(">>>> Shader `{}` <<<<", shader->m_path);
    std::string source;
    source.append((char *)output_code->getBufferPointer(), output_code->getBufferSize());
#endif

    return shader;
}

Result<> Shader::dump_glsl()
{
    // Re-use a global session to reduce load times.
    if (!s_global_session)
        slang::createGlobalSession(s_global_session.writeRef());

    slang::SessionDesc session_desc{};
    slang::TargetDesc target_desc{};

    std::vector<slang::CompilerOptionEntry> options;

    Slang::ComPtr<ISlangFileSystem> filesystem = Slang::ComPtr<ISlangFileSystem>(new UnimplementedFileSystem());
    session_desc.fileSystem = filesystem.get();

    // NOTE: If we use slang's default of row major matrices we will need to transpose matrices before passing them to shaders.
    options.push_back(slang::CompilerOptionEntry(slang::CompilerOptionName::MatrixLayoutColumn, slang::CompilerOptionValue(slang::CompilerOptionValueKind::Int, 1, 0, nullptr, nullptr)));

    target_desc.format = SLANG_GLSL;

    session_desc.targets = &target_desc;
    session_desc.targetCount = 1;

    session_desc.compilerOptionEntries = options.data();
    session_desc.compilerOptionEntryCount = options.size();

    std::vector<slang::PreprocessorMacroDesc> preprocessor_macro_desc{};
    session_desc.preprocessorMacros = preprocessor_macro_desc.data();
    session_desc.preprocessorMacroCount = (SlangInt)preprocessor_macro_desc.size();

    Slang::ComPtr<slang::ISession> session;

    SlangResult result = s_global_session->createSession(session_desc, session.writeRef());
    if (SLANG_FAILED(result))
        return Error(ErrorKind::ShaderCompilationFailed);

    Slang::ComPtr<slang::IBlob> diagnostics_blob;

    Slang::ComPtr<slang::IModule> module = Slang::ComPtr(session->loadModuleFromSourceString("shader", m_path.c_str(), m_source_code.data(), diagnostics_blob.writeRef()));
    if (!module)
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    std::vector<EntryPoint> entry_points;
    std::vector<slang::IComponentType *> component_types{module};

    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); i++)
    {
        Slang::ComPtr<slang::IEntryPoint> entry_point_comp;
        module->getDefinedEntryPoint(i, entry_point_comp.writeRef());

        // NOTE: This works but dont seems the best way to do it.
        slang::Attribute *attrib = entry_point_comp->getFunctionReflection()->findAttributeByName(s_global_session, "shader");
        size_t name_size = 0;
        std::string_view name(attrib->getArgumentValueString(0, &name_size));

        EntryPoint entry_point;
        entry_point.entry_point = entry_point_comp;
        entry_point.stage = stage_from_string(name);

        entry_points.push_back(entry_point);
        component_types.push_back(entry_point_comp);
    }

    // Compile the program, link and receive the SPIR-V bytecode.
    Slang::ComPtr<slang::IComponentType> composed_program;
    result = session->createCompositeComponentType(component_types.data(), (SlangInt)component_types.size(), composed_program.writeRef(), diagnostics_blob.writeRef());
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

    // Get all entry points in one blob.
    Slang::ComPtr<slang::IBlob> output_code;
    result = linked_program->getTargetCode(0, output_code.writeRef(), diagnostics_blob.writeRef());
    if (SLANG_FAILED(result))
    {
        println("{}", (char *)diagnostics_blob->getBufferPointer());
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    std::string s;
    s.append((char *)output_code->getBufferPointer(), output_code->getBufferSize());
    println("{}", s);

    return 0;
}
