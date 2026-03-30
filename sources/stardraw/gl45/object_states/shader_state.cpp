#include "shader_state.hpp"
#include <format>
#include <ranges>
#include <spirv_glsl.hpp>
#include "stardraw/gl45/api_conversion.hpp"
#include "stardraw/internal/internal.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    using namespace starlib_stdint;
    shader_state::shader_state(const shader& desc, status& out_status) : shader_id(desc.identifier())
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create shader object");
        out_status = create_from_stages(desc.stages);
    }

    shader_state::~shader_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete shader object");
        if (!is_valid()) return;

        glDeleteProgram(shader_program_id);
        shader_program_id = 0;
    }

    bool shader_state::is_valid() const
    {
        return shader_program_id != 0;
    }

    status shader_state::make_active() const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Make shader active");
        if (!is_valid()) return {status_type::BACKEND_ERROR, std::format("Shader object '{0}' not valid!", shader_id.name)};
        glUseProgram(shader_program_id);
        return status_type::SUCCESS;
    }

    status shader_state::dispatch_compute(const u32 groups_x, const u32 groups_y, const u32 groups_z) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Shader compute dispatch");
        if (!has_compute_stage) return {status_type::INVALID, std::format("Can't dispatch compute stage of shader '{0}' - doesn't have a compute stage!", shader_id.name)};
        status activate_status = make_active();
        if (activate_status.is_error()) return activate_status;
        glDispatchCompute(groups_x, groups_y, groups_z);
        return status_type::SUCCESS;
    }

    status shader_state::dispatch_compute_indirect(const u64 indirect_offset) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Shader compute dispatch (indirect)");
        if (!has_compute_stage) return {status_type::INVALID, std::format("Can't dispatch compute stage of shader '{0}' - doesn't have a compute stage!", shader_id.name)};
        status activate_status = make_active();
        if (activate_status.is_error()) return activate_status;
        glDispatchComputeIndirect(static_cast<GLintptr>(indirect_offset));
        return status_type::SUCCESS;
    }

    status shader_state::upload_parameter(const shader_parameter& parameter)
    {
        ZoneScoped;
        if (!parameter.location.internal->is_valid) return {status_type::UNKNOWN, std::format("Shader parameter location '{1}' not found in shader '{0}'", shader_id.name, parameter.location.internal->path_string)};
        const auto existing_param = std::ranges::find(parameter_store, parameter);
        if (existing_param == parameter_store.end()) parameter_store.push_back(parameter);
        else parameter_store.emplace(existing_param, parameter);
        return status_type::SUCCESS;
    }

    void shader_state::clear_parameters()
    {
        ZoneScoped;
        parameter_store.clear();
    }

    void shader_state::flag_barriers(memory_barrier_controller& barrier_controller) const
    {
        ZoneScoped;
        for (const object_binding& bound_object : bound_objects | std::views::values)
        {
            if (!bound_object.write_access) continue;
            barrier_controller.flag_barriers(bound_object.identifier);
        }
    }

    descriptor_type shader_state::object_type() const
    {
        return descriptor_type::SHADER;
    }

    void shader_state::barrier_objects_if_needed(memory_barrier_controller& barrier_controller) const
    {
        ZoneScoped;
        for (const object_binding& bound_object : bound_objects | std::views::values)
        {
            barrier_controller.barrier_if_needed(bound_object.identifier, bound_object.read_barriers);
        }
    }

    status shader_state::create_from_stages(const std::vector<shader_stage>& stages)
    {
        ZoneScoped;
        std::vector<std::string> converted_sources;
        status convert_status = remap_spirv_stages(stages, converted_sources);
        if (convert_status.is_error()) return convert_status;

        status stages_compile_status = status_type::SUCCESS;
        std::vector<GLuint> shader_stages;
        for (u32 idx = 0; idx < stages.size(); idx++)
        {
            const shader_stage& stage = stages[idx];
            if (stage.internal->type == shader_stage_type::COMPUTE)
            {
                has_compute_stage = true;
            }
            const GLenum shader_type = to_gl_shader_type(stage.internal->type);
            if (shader_type == 0)
            {
                stages_compile_status = {status_type::BACKEND_ERROR, std::format("A provided shader stage in the shader '{0}' is not supported on this API!", shader_id.name)};
                break;
            }

            const std::string& source = converted_sources[idx];
            GLuint compiled_stage;
            const status compile_status = compile_shader_stage(source, shader_type, compiled_stage);
            if (compile_status.is_error())
            {
                stages_compile_status = compile_status;
                break;
            }

            shader_stages.push_back(compiled_stage);
        }

        if (stages_compile_status.is_error())
        {
            for (const GLuint& compiled_stage : shader_stages)
            {
                glDeleteShader(compiled_stage);
            }

            return stages_compile_status;
        }

        GLuint shader_program;
        status link_status = link_shader(shader_stages, shader_program);

        for (const GLuint shader : shader_stages)
        {
            glDeleteShader(shader);
        }

        if (link_status.is_error()) return link_status;

        shader_program_id = shader_program;

        return status_type::SUCCESS;
    }


    status shader_state::remap_spirv_stages(const std::vector<shader_stage>& stages, std::vector<std::string>& out_sources)
    {
        ZoneScopedN("Convert Slang spirv to opengl-compatible GLSL");
        for (const shader_stage& stage : stages)
        {
            if (stage.internal->api != graphics_api::GL45) return {status_type::INVALID, std::format("A provided shader program for shader '{0}' is non-GL45!", shader_id.name)};
        }

        struct stage_compiler
        {
            spirv_cross::CompilerGLSL compiler;
            std::vector<spirv_cross::Resource> resources_with_binding_sets;
        };

        std::vector<stage_compiler*> stage_compilers;
        status result_status = status_type::SUCCESS;

        constexpr spirv_cross::CompilerGLSL::Options options = spirv_cross::CompilerGLSL::Options {
            .version = 450,
            .enable_storage_image_qualifier_deduction = false,
        };

        try
        {
            for (const shader_stage& stage : stages)
            {
                stage_compilers.push_back(new stage_compiler {spirv_cross::CompilerGLSL(static_cast<const u32*>(stage.internal->data), stage.internal->data_size / sizeof(u32)), {}});
            }

            std::vector<u32> bindings_per_set;

            for (stage_compiler* stage : stage_compilers)
            {
                stage->compiler.set_common_options(options);

                spirv_cross::ShaderResources resources = stage->compiler.get_shader_resources();

                stage->resources_with_binding_sets.append_range(resources.sampled_images);
                stage->resources_with_binding_sets.append_range(resources.separate_images);
                stage->resources_with_binding_sets.append_range(resources.separate_samplers);
                stage->resources_with_binding_sets.append_range(resources.uniform_buffers);
                stage->resources_with_binding_sets.append_range(resources.storage_buffers);
                stage->resources_with_binding_sets.append_range(resources.storage_images);
                stage->resources_with_binding_sets.append_range(resources.atomic_counters);

                for (const spirv_cross::Resource& resource : stage->resources_with_binding_sets)
                {
                    const u32 descriptor_set = stage->compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    const u32 binding_index = stage->compiler.get_decoration(resource.id, spv::DecorationBinding);
                    if (descriptor_set >= bindings_per_set.size()) bindings_per_set.resize(descriptor_set + 1);
                    bindings_per_set[descriptor_set] = std::max(bindings_per_set[descriptor_set], binding_index + 1);
                }
            }

            descriptor_set_binding_offsets.resize(bindings_per_set.size());

            u32 binding_offset = 0;
            for (u32 idx = 0; idx < bindings_per_set.size(); idx++)
            {
                descriptor_set_binding_offsets[idx] = binding_offset;
                binding_offset += bindings_per_set[idx];
            }

            for (stage_compiler* stage : stage_compilers)
            {
                for (const spirv_cross::Resource& resource : stage->resources_with_binding_sets)
                {
                    const u32 descriptor_set = stage->compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    const u32 binding_index = stage->compiler.get_decoration(resource.id, spv::DecorationBinding);
                    stage->compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
                    stage->compiler.set_decoration(resource.id, spv::DecorationBinding, binding_index + descriptor_set_binding_offsets[descriptor_set]);
                }

                //Handle merging sampler states with samplers where possible, transfer binding and name from original sampler.
                stage->compiler.build_combined_image_samplers();
                for (const spirv_cross::CombinedImageSampler& combined : stage->compiler.get_combined_image_samplers())
                {
                    const std::string tex_name = stage->compiler.get_name(combined.image_id);
                    const u32 binding = stage->compiler.get_decoration(combined.image_id, spv::Decoration::DecorationBinding);
                    stage->compiler.set_decoration(combined.combined_id, spv::Decoration::DecorationBinding, binding);
                    stage->compiler.set_name(combined.combined_id, tex_name);
                }

                stage->compiler.set_common_options({.version = 450, .emit_push_constant_as_uniform_buffer = true});

                const std::string source = stage->compiler.compile();
                if (source.empty())
                {
                    result_status = {status_type::BACKEND_ERROR, std::format("Failed to transpile SPIR-V into OpenGL compatible GLSL for shader '{0}'", shader_id.name)};
                    break;
                }

                out_sources.push_back(source);
            }
        }
        catch (std::exception& _)
        {
            result_status = {status_type::BACKEND_ERROR, std::format("Failed to transpile SPIR-V into OpenGL compatible GLSL for shader '{0}'", shader_id.name)};
        }

        for (const stage_compiler* stage : stage_compilers)
        {
            delete stage;
        }

        return result_status;
    }

    status shader_state::link_shader(const std::vector<GLuint>& stages, GLuint& out_shader_id)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Link shader stages");
        const GLuint program = glCreateProgram();
        if (program == 0) return {status_type::BACKEND_ERROR, std::format("Creating shader '{0}' failed (glCreateProgram)", shader_id.name)};

        for (const GLuint shader : stages)
        {
            if (shader != 0)
                glAttachShader(program, shader);
        }

        glLinkProgram(program);

        GLint success = GL_TRUE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (success != GL_TRUE)
        {
            std::string log = get_program_log(program);
            glDeleteProgram(program);
            return {status_type::BACKEND_ERROR, std::format("Shader validation for shader '{1}' failed with error: \n {0}", log, shader_id.name)};
        }

        out_shader_id = program;

        return status_type::SUCCESS;
    }

    status shader_state::compile_shader_stage(const std::string& source, const GLuint type, GLuint& out_shader_id)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Compile shader stage");
        const GLuint shader = glCreateShader(type);
        if (shader == 0) return {status_type::BACKEND_ERROR, std::format("Creating shader '{0}' failed (glCreateShader)", shader_id.name)};

        const char* c_str = source.c_str();
        glShaderSource(shader, 1, &c_str, nullptr);
        glCompileShader(shader);

        GLint success = GL_TRUE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (success != GL_TRUE)
        {
            const std::string log = get_shader_log(shader);
            glDeleteShader(shader);
            return {status_type::BACKEND_ERROR, std::format("Shader stage compilation for shader '{1}' failed with error: \n {0}", log, shader_id.name)};
        }

        out_shader_id = shader;

        return status_type::SUCCESS;
    }

    status shader_state::validate_program(const GLuint program)
    {
        ZoneScoped;
        GLint success = GL_TRUE;

        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);

        if (success != GL_TRUE)
        {
            return {status_type::BACKEND_ERROR, std::format("Shader validation for shader '{1}' failed with error: \n {0}", get_program_log(program), shader_id.name)};
        }

        return status_type::SUCCESS;
    }

    std::string shader_state::get_shader_log(const GLuint shader) const
    {
        ZoneScoped;
        i32 log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        std::string log;
        log.resize(std::max(log_length, 0));

        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, log.length(), log.data());

        return log;
    }

    std::string shader_state::get_program_log(const GLuint program) const
    {
        ZoneScoped;
        i32 log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        std::string log;
        log.resize(std::max(log_length, 0));

        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, log.length(), log.data());

        return log;
    }
}