/*
    ./shadercompiler <output folder> [files...]

    `shadercompiler` is a wrapper around dawn's shader compiler tint.
    It takes a GLSL file and convert it to SPIR-V for most platform or WGSL for the web and a
    <input file>.map.json describing important info about the shader.
    It will automatically detect shaders contaning multiple variants and will compile to as many shaders as
    there is variants.

    Implementation notes:
    - On Linux and Windows compiles directly to SPIR-V for Vulkan using glslang.
    - On MacOS compiles first to SPIR-V with glslang, then to MSL with SPIRV-Cross. (TODO)
    - On Web compiles first to SPIR-V with glslang, then to WGSL with Dawn. (TODO)
 */

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

// #include <src/tint/lang/spirv/writer/helpers/generate_bindings.h>
// #include <src/tint/lang/spirv/writer/printer/printer.h>
// #include <src/tint/lang/spirv/writer/raise/raise.h>

// #include <src/tint/lang/core/ir/var.h>
// #include <src/tint/lang/core/type/pointer.h>
// #include <src/tint/lang/core/type/storage_texture.h>
// #include <src/tint/lang/wgsl/ast/module.h>

#include <nlohmann/json.hpp>

#include <glslang/Include/intermediate.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include "Render/ShaderMap.hpp"

using namespace glslang;

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

static EShLanguage detect_stage(const std::filesystem::path& file)
{
    if (file.extension() == ".vert")
        return EShLangVertex;
    else if (file.extension() == ".frag")
        return EShLangFragment;
    else if (file.extension() == ".comp")
        return EShLangCompute;
    return EShLangCount;
}

static std::string stage_extension(EShLanguage stage)
{
    switch (stage)
    {
    case EShLangVertex:
        return ".vert";
    case EShLangFragment:
        return ".frag";
    case EShLangCompute:
        return ".comp";
    default:
        return "";
    }
}

static std::string read_file(const std::filesystem::path& filepath)
{
    std::ifstream ifs(filepath, std::ifstream::binary | std::ifstream::ate);
    std::string buffer;
    buffer.resize(ifs.tellg());
    ifs.seekg(0);

    ifs.read(buffer.data(), (std::streamsize)buffer.size());
    return buffer;
}

static inline BindingKind convert_uniform_type(const char *s)
{
    nlohmann::json j = s;
    return j;
}

static void compile_shaders(ShaderVariant variant, ShaderMap& map, const std::vector<std::string>& files, const std::filesystem::path& output_folder, const std::filesystem::path& base_path);
static void emit_spirv(TShader *shader, const std::filesystem::path& path);
static ShaderStageFlags convert_stage_mask(EShLanguageMask mask);
static ShaderStageFlags convert_stage(EShLanguage mask);

struct Variant
{
    std::string name;
    std::vector<std::string> files;
};

int main(int argc, char *argv[])
{
    std::filesystem::path output_folder = argv[1];
    std::filesystem::path base_path = argv[2];

    std::map<std::string, Variant> variants;
    ShaderMap map{};

    for (int i = 3; i < argc; i++)
    {
        i++;

        Variant variant{};
        variant.name = argv[i++];

        for (; i < argc && std::strcmp(argv[i], "--variant"); i++)
        {
            variant.files.push_back(argv[i]);
        }

        variants[variant.name] = variant;
    }

    for (const auto& [key, value] : variants)
    {
        // compile_shaders(ShaderVariant variant, map, value.files, output_folder, base_path);
    }

    // std::filesystem::path output_folder = argv[1];
    // std::filesystem::path base_path = argv[2];
    // std::vector<std::string> files;

    // std::array<const char *, 3> extensions = {"vert", "frag", "comp"};

    // for (const char *extension : extensions)
    // {
    //     std::filesystem::path filepath = base_path;
    //     filepath.replace_extension(extension);

    //     if (std::filesystem::exists(filepath))
    //         files.push_back(filepath.string());
    // }

    // ShaderMap map{};

    // compile_shaders(ShaderVariant::Base, map, files, output_folder, base_path);

    // // Write the map file.
    // std::filesystem::path map_filepath = output_folder;
    // map_filepath.append(base_path.string());
    // map_filepath.replace_extension(".map.json");

    // std::filesystem::create_directories(map_filepath.parent_path());

    // nlohmann::json j = map;

    // std::ofstream os(map_filepath);
    // assert(os.is_open() && "cannot open file for writing");
    // os << nlohmann::to_string(j);
}

static void compile_shaders(ShaderVariant variant, ShaderMap& map, const std::vector<std::string>& files, const std::filesystem::path& output_folder, const std::filesystem::path& base_path)
{
    std::vector<TShader *> shaders;
    TProgram *program = (new TProgram());

    for (const std::string& file : files)
    {
        EShLanguage stage = detect_stage(file);
        std::string source = read_file(file);

        TShader *shader = new TShader(detect_stage(file));
        const char *text[] = {source.c_str()};
        const char *filename[] = {file.c_str()};
        shader->setStringsWithLengthsAndNames(text, nullptr, filename, 1);
        shader->setEntryPoint("main");
        shader->setEnvClient(EShClientVulkan, EShTargetVulkan_1_2);
        shader->setEnvTarget(EShTargetSpv, EShTargetSpv_1_5); // SPIRV 1.5 is maximum for Vulkan 1.2

        TBuiltInResource resources = DefaultTBuiltInResource;
        resources.maxDrawBuffers = 1;

        TShader::ForbidIncluder includer; // TODO: Support includes.

        if (!shader->parse(&resources, 110, false, EShMsgDefault, includer))
        {
            std::cerr << shader->getInfoLog() << "\n";
            return;
        }

        shaders.push_back(shader);
        program->addShader(shader);
    }

    program->link(EShMsgDefault);
    program->buildReflection();

    ShaderMap::Variant variant_s{};

    // Write the map file.
    for (size_t i = 0; i < files.size(); i++)
    {
        TShader *shader = shaders[i];

        std::filesystem::path path = files[i];
        path.replace_extension(stage_extension(shader->getStage()) + ".spv");

        variant_s.stages.push_back(ShaderMap::Stage{.stage = convert_stage(shader->getStage()), .file = path});
    }

    for (int i = 0; i < program->getNumUniformVariables(); i++)
    {
        const TObjectReflection& object = program->getUniform(i);

        // Each field in a uniform block is also in the uniform variable list with a binding of -1 so
        // we skip them.
        if (object.getBinding() == -1)
        {
            continue;
        }

        ShaderMap::Uniform uniform{};
        uniform.name = object.name;
        uniform.type = convert_uniform_type(object.getType()->getBasicTypeString().c_str());
        uniform.set = object.getType()->getQualifier().layoutSet;
        uniform.binding = object.getBinding();
        uniform.stage = convert_stage_mask(object.stages);

        map.uniforms.push_back(uniform);
    }

    for (int i = 0; i < program->getNumUniformBlocks(); i++)
    {
        const TObjectReflection& object = program->getUniformBlock(i);

        ShaderMap::Uniform uniform{};
        uniform.name = object.name;
        uniform.type = object.getType()->getQualifier().layoutPushConstant ? BindingKind::PushConstants : BindingKind::UniformBuffer;
        uniform.set = object.getType()->getQualifier().layoutSet;
        uniform.binding = object.getBinding();
        uniform.stage = convert_stage_mask(object.stages);

        map.uniforms.push_back(uniform);
    }

    // Emit SPIR-V for each shaders.
    for (size_t i = 0; i < files.size(); i++)
    {
        std::filesystem::path path = output_folder;
        path.append(files[i]);

        emit_spirv(shaders[i], path);
    }

    map.variants.push_back(variant_s);
}

static void emit_spirv(TShader *shader, const std::filesystem::path& path)
{
    SpvOptions options;
    options.disableOptimizer = false;

    std::vector<uint32_t> bytecode;
    GlslangToSpv(*shader->getIntermediate(), bytecode, &options);

    std::filesystem::path output_path = path;
    output_path.replace_extension(stage_extension(shader->getStage()) + ".spv");

    std::filesystem::create_directories(output_path.parent_path());

    std::ofstream os(output_path);
    assert(os.is_open() && "cannot open file for writing");
    os.write((char *)bytecode.data(), (std::streamsize)(bytecode.size() * sizeof(uint32_t)));
}

static ShaderStageFlags convert_stage_mask(EShLanguageMask mask)
{
    ShaderStageFlags flags;

    if (mask & EShLangVertexMask)
        flags |= ShaderStageFlagBits::Vertex;
    else if (mask & EShLangFragmentMask)
        flags |= ShaderStageFlagBits::Fragment;
    else if (mask & EShLangComputeMask)
        flags |= ShaderStageFlagBits::Compute;

    return flags;
}

static ShaderStageFlags convert_stage(EShLanguage mask)
{
    ShaderStageFlags flags;

    if (mask == EShLangVertex)
        flags |= ShaderStageFlagBits::Vertex;
    else if (mask == EShLangFragment)
        flags |= ShaderStageFlagBits::Fragment;
    else if (mask == EShLangCompute)
        flags |= ShaderStageFlagBits::Compute;

    return flags;
}
