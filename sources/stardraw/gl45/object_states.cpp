#include "object_states.hpp"

#include <format>
#include <spirv_glsl.hpp>

#include "stardraw/api/memory_transfer.hpp"
#include "stardraw/internal/internal.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{


    buffer_state::buffer_state(const buffer_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create buffer object");

        buffer_name = desc.identifier().name;

        glCreateBuffers(1, &main_buffer_id);
        if (main_buffer_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating buffer {0} failed", desc.identifier().name)};
            return;
        }

        const GLbitfield flags = (desc.memory == buffer_memory_storage::SYSRAM) ? GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT : 0;

        main_buffer_size = desc.size;
        glNamedBufferStorage(main_buffer_id, main_buffer_size, nullptr, flags);
        out_status = status_type::SUCCESS;
    }

    buffer_state::~buffer_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete buffer object");
        glDeleteBuffers(1, &main_buffer_id);
    }

    descriptor_type buffer_state::object_type() const
    {
        return descriptor_type::BUFFER;
    }

    bool buffer_state::is_valid() const
    {
        return main_buffer_id != 0;
    }

    status buffer_state::bind_to(const GLenum target) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind buffer");
        glBindBuffer(target, main_buffer_id);
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind buffer (slot binding)");
        glBindBufferRange(target, slot, main_buffer_id, 0, main_buffer_size);
        return status_type::SUCCESS;
    }

    status buffer_state::bind_to_slot(const GLenum target, const GLuint slot, const GLintptr address, const GLsizeiptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind buffer (slot binding)");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested bind range is out of range in buffer '{0}'", buffer_name)};
        glBindBufferRange(target, slot, main_buffer_id, address, bytes);
        return status_type::SUCCESS;
    }

    /*
    status buffer_state::upload_data_direct(const GLintptr address, const void* const data, const GLsizeiptr bytes)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Direct buffer upload");
        if (data == nullptr) return {status_type::UNEXPECTED, "Data pointer was null!"};
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        if (main_buff_pointer == nullptr)
        {
            const status map_status = map_main_buffer();
            if (is_status_error(map_status)) return map_status;
        }

        const GLbyte* source_pointer = static_cast<const GLbyte*>(data);
        GLbyte* dest_pointer = static_cast<GLbyte*>(main_buff_pointer);
        memcpy(dest_pointer + address, source_pointer, bytes);
        return status_type::SUCCESS;
    }
    */

    status buffer_state::prepare_upload_data_streaming(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare direct buffer upload");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        gl_memory_transfer_handle* staged_handle = nullptr;
        status alloc_status = staging_uploader.allocate_upload(address, bytes, main_buffer_size, &staged_handle);
        if (is_status_error(alloc_status)) return alloc_status;
        *out_handle = staged_handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_streaming(memory_transfer_handle* handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Flush staged buffer upload");
        const gl_memory_transfer_handle* staged_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (staged_handle == nullptr) return {status_type::INVALID, "Invalid memory transfer handle cast - this is an internal bug!"};
        status copy_status = copy_data(staged_handle->transfer_buffer_id, staged_handle->transfer_buffer_address, staged_handle->transfer_destination_address, staged_handle->transfer_size);
        if (is_status_error(copy_status)) return copy_status;
        return staging_buffer_uploader::flush_upload(staged_handle);
    }

    status buffer_state::prepare_upload_data_chunked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare staged buffer upload");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        GLuint temp_buffer;
        glCreateBuffers(1, &temp_buffer);
        if (temp_buffer == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create temporary upload destination for buffer '{0}'", buffer_name)};

        glNamedBufferStorage(temp_buffer, bytes, nullptr, GL_MAP_WRITE_BIT);
        GLbyte* temp_buffer_ptr = static_cast<GLbyte*>(glMapNamedBuffer(temp_buffer, GL_WRITE_ONLY));
        if (temp_buffer_ptr == nullptr)
        {
            glDeleteBuffers(1, &temp_buffer);
            return {status_type::BACKEND_ERROR, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_name)};
        }

        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_size = bytes;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = temp_buffer;
        handle->transfer_buffer_ptr = temp_buffer_ptr;
        handle->transfer_buffer_address = 0;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_chunked(memory_transfer_handle* handle) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Flush staged buffer upload");
        const gl_memory_transfer_handle* chunked_handle = dynamic_cast<gl_memory_transfer_handle*>(handle);
        if (chunked_handle == nullptr) return {status_type::INVALID, "Invalid memory transfer handle cast - this is an internal bug!"};
        glUnmapNamedBuffer(chunked_handle->transfer_buffer_id);
        status copy_status = copy_data(chunked_handle->transfer_buffer_id, chunked_handle->transfer_buffer_address, chunked_handle->transfer_destination_address, chunked_handle->transfer_size);
        glDeleteBuffers(1, &chunked_handle->transfer_buffer_id);
        delete handle;
        return copy_status;
    }

    status buffer_state::prepare_upload_data_unchecked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Prepare direct buffer upload");
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        if (main_buff_pointer == nullptr)
        {
            status map_status = map_main_buffer();
            if (is_status_error(map_status)) return map_status;
        }

        gl_memory_transfer_handle* handle = new gl_memory_transfer_handle();
        handle->transfer_size = bytes;
        handle->transfer_destination_address = address;
        handle->transfer_buffer_id = main_buffer_size;
        handle->transfer_buffer_ptr = main_buff_pointer;
        handle->transfer_buffer_address = 0;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    status buffer_state::flush_upload_data_unchecked(const memory_transfer_handle* handle)
    {
        delete handle;
        return status_type::SUCCESS;
    }

    status buffer_state::copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Buffer data transfer");

        if (!is_in_buffer_range(read_address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};
        glCopyNamedBufferSubData(source_buffer_id, main_buffer_id, read_address, write_address, bytes);
        return status_type::SUCCESS;
    }

    GLsizeiptr buffer_state::get_size() const
    {
        return main_buffer_size;
    }

    bool buffer_state::is_in_buffer_range(const GLintptr address, const GLsizeiptr size) const
    {
        return address + size <= get_size();
    }

    GLuint buffer_state::gl_id() const
    {
        return main_buffer_id;
    }

    status buffer_state::map_main_buffer()
    {
        if (main_buff_pointer != nullptr) return status_type::NOTHING_TO_DO;
        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        main_buff_pointer = glMapNamedBufferRange(main_buffer_id, 0, main_buffer_size, flags);
        if (main_buff_pointer == nullptr) return {status_type::BACKEND_ERROR, std::format("Unable to write directly to buffer '{0}' (you probably need to create it with the SYSRAM memory hint?)", buffer_name)};
        return status_type::SUCCESS;
    }

    shader_state::shader_state(const shader_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create shader object");
        out_status = create_from_stages(desc.stages);
    }

    shader_state::~shader_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete shader object")
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
        if (!is_valid()) return {status_type::BACKEND_ERROR, "Shader object not valid!"};
        glUseProgram(shader_program_id);
        return status_type::SUCCESS;
    }

    status shader_state::upload_parameter(const shader_parameter& parameter)
    {
        if (parameter.location == invalid_shader_paramter_location) return {status_type::UNKNOWN, "Shader parameter location not found in shader"};
        const auto existing_param = std::ranges::find(parameter_store, parameter);
        if (existing_param == parameter_store.end()) parameter_store.push_back(parameter);
        else parameter_store.emplace(existing_param, parameter);
        return status_type::SUCCESS;
    }

    void shader_state::clear_parameters()
    {
        parameter_store.clear();
    }

    descriptor_type shader_state::object_type() const
    {
        return descriptor_type::SHADER;
    }

    status shader_state::create_from_stages(const std::vector<shader_stage>& stages)
    {
        std::vector<std::string> converted_sources;
        status convert_status = remap_spirv_stages(stages, converted_sources);
        if (is_status_error(convert_status)) return convert_status;

        status stages_compile_status = status_type::SUCCESS;
        std::vector<GLuint> shader_stages;
        for (uint32_t idx = 0; idx < stages.size(); idx++)
        {
            const shader_stage& stage = stages[idx];

            const GLenum shader_type = gl_shader_type(stage.type);
            if (shader_type == 0)
            {
                stages_compile_status = {status_type::BACKEND_ERROR, "A provided shader stage is not supported on this API!"};
                break;
            }

            const std::string& source = converted_sources[idx];
            GLuint compiled_stage;
            const status compile_status = compile_shader_stage(source, shader_type, compiled_stage);
            if (is_status_error(compile_status))
            {
                stages_compile_status = compile_status;
                break;
            }

            shader_stages.push_back(compiled_stage);
        }

        if (is_status_error(stages_compile_status))
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

        if (is_status_error(link_status)) return link_status;

        shader_program_id = shader_program;

        return status_type::SUCCESS;
    }

    GLenum shader_state::gl_shader_type(const shader_stage_type stage)
    {
        switch (stage)
        {
            case shader_stage_type::VERTEX: return GL_VERTEX_SHADER;
            case shader_stage_type::TESSELATION_CONTROL: return GL_TESS_CONTROL_SHADER;
            case shader_stage_type::TESSELATION_EVAL: return GL_TESS_EVALUATION_SHADER;
            case shader_stage_type::GEOMETRY: return GL_GEOMETRY_SHADER;
            case shader_stage_type::FRAGMENT: return GL_FRAGMENT_SHADER;
            case shader_stage_type::COMPUTE: return GL_COMPUTE_SHADER;
        }

        return 0;
    }

    status shader_state::remap_spirv_stages(const std::vector<shader_stage>& stages, std::vector<std::string>& out_sources)
    {
        for (const shader_stage& stage : stages)
        {
            if (stage.program->api != graphics_api::GL45) return {status_type::INVALID, std::format("A provided shader program is non-GL45!")};
        }

        struct stage_compiler
        {
            spirv_cross::CompilerGLSL compiler;
            std::vector<spirv_cross::Resource> resources_with_binding_sets;
        };

        std::vector<stage_compiler*> stage_compilers;
        status result_status = status_type::SUCCESS;

        try
        {
            for (const shader_stage& stage : stages)
            {
                stage_compilers.push_back(new stage_compiler {spirv_cross::CompilerGLSL(static_cast<const uint32_t*>(stage.program->data), stage.program->data_size / sizeof(uint32_t)), {}});
            }

            std::vector<uint32_t> bindings_per_set;

            for (stage_compiler* stage : stage_compilers)
            {
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
                    const uint32_t descriptor_set = stage->compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    const uint32_t binding_index = stage->compiler.get_decoration(resource.id, spv::DecorationBinding);
                    if (descriptor_set >= bindings_per_set.size()) bindings_per_set.resize(descriptor_set + 1);
                    bindings_per_set[descriptor_set] = std::max(bindings_per_set[descriptor_set], binding_index + 1);
                }
            }

            descriptor_set_binding_offsets.resize(bindings_per_set.size());

            uint32_t binding_offset = 0;
            for (uint32_t idx = 0; idx < bindings_per_set.size(); idx++)
            {
                descriptor_set_binding_offsets[idx] = binding_offset;
                binding_offset += bindings_per_set[idx];
            }

            for (stage_compiler* stage : stage_compilers)
            {
                for (const spirv_cross::Resource& resource : stage->resources_with_binding_sets)
                {
                    const uint32_t descriptor_set = stage->compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    const uint32_t binding_index = stage->compiler.get_decoration(resource.id, spv::DecorationBinding);
                    stage->compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
                    stage->compiler.set_decoration(resource.id, spv::DecorationBinding, binding_index + descriptor_set_binding_offsets[descriptor_set]);
                }

                //Handle merging sampler states with samplers where possible, transfer binding and name from original sampler.
                stage->compiler.build_combined_image_samplers();
                for (const spirv_cross::CombinedImageSampler& combined : stage->compiler.get_combined_image_samplers())
                {
                    const std::string tex_name = stage->compiler.get_name(combined.image_id);
                    const uint32_t binding = stage->compiler.get_decoration(combined.image_id, spv::Decoration::DecorationBinding);
                    stage->compiler.set_decoration(combined.combined_id, spv::Decoration::DecorationBinding, binding);
                    stage->compiler.set_name(combined.combined_id, tex_name);
                }

                stage->compiler.set_common_options({.version = 450, .emit_push_constant_as_uniform_buffer = true});

                const std::string source = stage->compiler.compile();
                if (source.empty())
                {
                    result_status = {status_type::BACKEND_ERROR, "Failed to transpile SPIR-V into OpenGL compatible GLSL"};
                    break;
                }

                out_sources.push_back(source);
            }
        }
        catch (std::exception& _)
        {
            result_status = {status_type::BACKEND_ERROR, "Failed to transpile SPIR-V into OpenGL compatible GLSL"};
        }

        for (const stage_compiler* stage : stage_compilers)
        {
            delete stage;
        }

        return result_status;
    }

    status shader_state::link_shader(const std::vector<GLuint>& stages, GLuint& out_shader_id)
    {
        const GLuint program = glCreateProgram();
        if (program == 0) return {status_type::BACKEND_ERROR, "Creating shader failed (glCreateProgram)"};

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
            glDeleteProgram(program);
            return {status_type::BACKEND_ERROR, std::format("Shader validation failed with error: \n {0}", get_program_log(program))};
        }

        out_shader_id = program;

        return status_type::SUCCESS;
    }

    status shader_state::compile_shader_stage(const std::string& source, const GLuint type, GLuint& out_shader_id)
    {
        const GLuint shader = glCreateShader(type);
        if (shader == 0) return {status_type::BACKEND_ERROR, "Creating shader failed (glCreateShader)"};

        const char* c_str = source.c_str();
        glShaderSource(shader, 1, &c_str, nullptr);
        glCompileShader(shader);

        GLint success = GL_TRUE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (success != GL_TRUE)
        {
            const std::string log = get_shader_log(shader);
            glDeleteShader(shader);
            return {status_type::BACKEND_ERROR, std::format("Shader stage compilation failed with error: \n {0}", log)};
        }

        out_shader_id = shader;

        return status_type::SUCCESS;
    }

    status shader_state::validate_program(const GLuint program)
    {
        GLint success = GL_TRUE;

        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &success);

        if (success != GL_TRUE)
        {
            return {status_type::BACKEND_ERROR, std::format("Shader validation failed with error: \n {0}", get_program_log(program))};
        }

        return status_type::SUCCESS;
    }

    std::string shader_state::get_shader_log(const GLuint shader)
    {
        int32_t log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        std::string log;
        log.resize(std::max(log_length, 0));

        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, log.length(), log.data());

        return log;
    }

    std::string shader_state::get_program_log(const GLuint program)
    {
        int32_t log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        std::string log;
        log.resize(std::max(log_length, 0));

        glGetProgramInfoLog(program, log_length, nullptr, log.data());
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 0, GL_DEBUG_SEVERITY_HIGH, log.length(), log.data());

        return log;
    }

    vertex_specification_state::vertex_specification_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create vertex specification");
        glCreateVertexArrays(1, &vertex_array_id);
    }

    vertex_specification_state::~vertex_specification_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete vertex specification");
        glDeleteVertexArrays(1, &vertex_array_id);
    }

    bool vertex_specification_state::is_valid() const
    {
        ZoneScoped;
        if (vertex_array_id == 0) return false;
        for (const GLuint buffer : vertex_buffers)
        {
            if (!glIsBuffer(buffer)) return false;
        }

        if (index_buffer != 0 && !glIsBuffer(index_buffer)) return false;

        return true;
    }

    bool vertex_specification_state::has_index_buffer() const
    {
        return index_buffer != 0;
    }

    status vertex_specification_state::bind() const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind vertex specification");
        glBindVertexArray(vertex_array_id);
        return status_type::SUCCESS;
    }

    status vertex_specification_state::attach_vertex_buffer(const GLuint slot, const GLuint id, const GLintptr offset, const GLsizei stride)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Attach vertex buffer to vertex specification");
        glVertexArrayVertexBuffer(vertex_array_id, slot, id, offset, stride);
        vertex_buffers.push_back(id);
        return status_type::SUCCESS;
    }

    status vertex_specification_state::attach_index_buffer(const GLuint index_buffer_id)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Attach index buffer to vertex specification");
        glVertexArrayElementBuffer(vertex_array_id, index_buffer_id);
        index_buffer = index_buffer_id;
        return status_type::SUCCESS;
    }

    draw_specification_state::draw_specification_state(const draw_specification_descriptor& descriptor, const bool has_index_buffer) : shader(descriptor.shader), vertex_specification(descriptor.vertex_specification), has_index_buffer(has_index_buffer) {}

    descriptor_type draw_specification_state::object_type() const
    {
        return descriptor_type::DRAW_SPECIFICATION;
    }

    GLenum texture_type_to_gl_target(const texture_shape type, const bool multisample, const bool is_array)
    {
        switch (type)
        {
            case texture_shape::_1D: return is_array ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;
            case texture_shape::_2D: return multisample ? (is_array ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE) : (is_array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D);
            case texture_shape::_3D: return GL_TEXTURE_3D;
            case texture_shape::CUBE_MAP: return is_array ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
        }
        return 0;
    }

    GLenum texture_data_type_to_gl_format(const texture_data_type type)
    {
        switch (type)
        {
            case texture_data_type::DEPTH_32: return GL_DEPTH_COMPONENT32;
            case texture_data_type::DEPTH_24: return GL_DEPTH_COMPONENT24;
            case texture_data_type::DEPTH_16: return GL_DEPTH_COMPONENT16;
            case texture_data_type::DEPTH_32_STENCIL_8: return GL_DEPTH32F_STENCIL8;
            case texture_data_type::DEPTH_24_STENCIL_8: return GL_DEPTH24_STENCIL8;
            case texture_data_type::STENCIL_8: return GL_STENCIL_INDEX8;
            case texture_data_type::R_8: return GL_R8;
            case texture_data_type::RG_8: return GL_RG8;
            case texture_data_type::RGB_8: return GL_RGB8;
            case texture_data_type::RGBA_8: return GL_RGBA8;
            case texture_data_type::SRGB_8: return GL_SRGB8;
            case texture_data_type::SRGBA_8: return GL_SRGB8_ALPHA8;
            case texture_data_type::R_U8: return GL_R8UI;
            case texture_data_type::RG_U8: return GL_RG8UI;
            case texture_data_type::RGB_U8: return GL_RGB8UI;
            case texture_data_type::RGBA_U8: return GL_RGBA8UI;
            case texture_data_type::R_U16: return GL_R16UI;
            case texture_data_type::RG_U16: return GL_RG16UI;
            case texture_data_type::RGB_U16: return GL_RGB16UI;
            case texture_data_type::RGBA_U16: return GL_RGBA16UI;
            case texture_data_type::R_U32: return GL_R32UI;
            case texture_data_type::RG_U32: return GL_RG32UI;
            case texture_data_type::RGB_U32: return GL_RGB32UI;
            case texture_data_type::RGBA_U32: return GL_RGBA32UI;
            case texture_data_type::R_I8: return GL_R8I;
            case texture_data_type::RG_I8: return GL_RG8I;
            case texture_data_type::RGB_I8: return GL_RGB8I;
            case texture_data_type::RGBA_I8: return GL_RGBA8I;
            case texture_data_type::R_I16: return GL_R16I;
            case texture_data_type::RG_I16: return GL_RG16I;
            case texture_data_type::RGB_I16: return GL_RGB16I;
            case texture_data_type::RGBA_I16: return GL_RGBA16I;
            case texture_data_type::R_I32: return GL_R32I;
            case texture_data_type::RG_I32: return GL_RG32I;
            case texture_data_type::RGB_I32: return GL_RGB32I;
            case texture_data_type::RGBA_I32: return GL_RGBA32I;
            case texture_data_type::R_F16: return GL_R16F;
            case texture_data_type::RG_F16: return GL_RG16F;
            case texture_data_type::RGB_F16: return GL_RGB16F;
            case texture_data_type::RGBA_F16: return GL_RGBA16F;
            case texture_data_type::R_F32: return GL_R32F;
            case texture_data_type::RG_F32: return GL_RG32F;
            case texture_data_type::RGB_F32: return GL_RGB32F;
            case texture_data_type::RGBA_F32: return GL_RGBA32F;
        }

        return 0;
    }

    bool is_texture_data_type_integer(const texture_data_type type)
    {
        switch (type)
        {
            case texture_data_type::DEPTH_32_STENCIL_8:
            case texture_data_type::DEPTH_24_STENCIL_8:
            case texture_data_type::R_U8:
            case texture_data_type::RG_U8:
            case texture_data_type::RGB_U8:
            case texture_data_type::RGBA_U8:
            case texture_data_type::R_U16:
            case texture_data_type::RG_U16:
            case texture_data_type::RGB_U16:
            case texture_data_type::RGBA_U16:
            case texture_data_type::R_U32:
            case texture_data_type::RG_U32:
            case texture_data_type::RGB_U32:
            case texture_data_type::RGBA_U32:
            case texture_data_type::R_I8:
            case texture_data_type::RG_I8:
            case texture_data_type::RGB_I8:
            case texture_data_type::RGBA_I8:
            case texture_data_type::R_I16:
            case texture_data_type::RG_I16:
            case texture_data_type::RGB_I16:
            case texture_data_type::RGBA_I16:
            case texture_data_type::R_I32:
            case texture_data_type::RG_I32:
            case texture_data_type::RGB_I32:
            case texture_data_type::RGBA_I32:
            {
                return true;
            }

            default:
            {
                return false;
            }
        }
    }

    bool is_sampling_config_valid_for_type(const texture_data_type type, const texture_sampling_conifg& sampling_config)
    {
        const bool is_integer_type = is_texture_data_type_integer(type);
        if (!is_integer_type) return true;

        if (sampling_config.upscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.downscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.mipmap_filter != texture_filtering_mode::NEAREST) return false;

        return true;
    }

    texture_state::texture_state(const texture_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create texture object");

        num_texture_mipmap_levels = desc.format.mipmap_levels;
        num_texture_array_layers = desc.format.texture_layers;

        const bool has_msaa = desc.format.msaa != texture_msaa_level::NONE;
        const bool can_type_be_array = desc.format.shape != texture_shape::_3D;
        const bool can_shape_be_multisample = desc.format.shape == texture_shape::_2D || desc.format.shape == texture_shape::_3D;
        const bool is_array = (desc.format.shape != texture_shape::CUBE_MAP && num_texture_array_layers > 1) || num_texture_array_layers > 6;

        gl_texture_target = texture_type_to_gl_target(desc.format.shape, has_msaa, is_array);
        gl_texture_format = texture_data_type_to_gl_format(desc.format.data_type);

        const uint32_t width = desc.format.width;
        const uint32_t height = desc.format.height;
        const uint32_t depth = desc.format.depth;
        const uint8_t multisample_count = static_cast<uint8_t>(desc.format.msaa);

        if (num_texture_mipmap_levels <= 0)
        {
            out_status = {status_type::INVALID, std::format("Texture '{0}' has zero mipmap layers", desc.identifier().name)};
            return;
        }

        if (num_texture_array_layers <= 0)
        {
            out_status = {status_type::INVALID, std::format("Texture '{0}' has zero texture layers", desc.identifier().name)};
            return;
        }

        if (!can_shape_be_multisample && has_msaa)
        {
            out_status = {status_type::INVALID, std::format("Texture '{0}' shape cannot have MSAA, but msaa level is not zero", desc.identifier().name)};
            return;
        }

        if (!can_type_be_array && is_array)
        {
            out_status = {status_type::INVALID, std::format("Texture '{0}' shape cannot be an array texture, but has more than 1 texture layer", desc.identifier().name)};
            return;
        }

        if (desc.format.shape == texture_shape::CUBE_MAP && num_texture_array_layers % 6 != 0)
        {
            out_status = {status_type::INVALID, std::format("Texture '{0}' is a cubemap array texture, but number of texture layers is not a multiple of 6", desc.identifier().name)};
            return;
        }

        if (!is_sampling_config_valid_for_type(desc.format.data_type, desc.default_sampling_config))
        {
            out_status = {status_type::INVALID, std::format("Provided default sampling config isn't valid for texture '{0}' (are you trying to use linear filtering on an integer texture?)", desc.identifier().name)};
            return;
        }

        glCreateTextures(gl_texture_target, 1, &texture_id);

        if (texture_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating texture {0} failed", desc.identifier().name)};
            return;
        }

        switch (desc.format.shape)
        {
            case texture_shape::_1D:
            {
                if (num_texture_array_layers > 1) glTextureStorage2D(texture_id, num_texture_mipmap_levels, gl_texture_format, width, num_texture_array_layers);
                else glTextureStorage1D(texture_id, num_texture_mipmap_levels, gl_texture_format, width);
                break;
            }
            case texture_shape::_3D:
            {
                glTextureStorage3D(texture_id, num_texture_mipmap_levels, gl_texture_format, width, height, depth);
                break;
            }
            case texture_shape::_2D:
            case texture_shape::CUBE_MAP:
            {
                if (num_texture_array_layers > 1 && has_msaa)
                {
                    //It's unclear whether the last parameter (fixed sample locations) is actually used by any implementations, so for now I'm not supporting customizing it.
                    glTextureStorage3DMultisample(texture_id, multisample_count, gl_texture_format, width, height, num_texture_array_layers, GL_TRUE);
                    break;
                }

                if (num_texture_array_layers > 1)
                {
                    glTextureStorage3D(texture_id, num_texture_mipmap_levels, gl_texture_format, width, height, num_texture_array_layers);
                    break;
                }

                if (has_msaa)
                {
                    //It's unclear whether the last parameter (fixed sample locations) is actually used by any implementations, so for now I'm not supporting customizing it.
                    glTextureStorage2DMultisample(texture_id, multisample_count, gl_texture_format, width, height, GL_TRUE);
                    break;
                }

                glTextureStorage2D(texture_id, num_texture_mipmap_levels, gl_texture_format, width, height);
                break;
            }
        }

        texture_sampling_conifg sampling_config = desc.default_sampling_config;
        sampling_config.mipmap_max_level = std::min(sampling_config.mipmap_max_level, num_texture_mipmap_levels - 1);
        out_status = set_sampling_config(sampling_config);
    }

    texture_state::texture_state(const texture_state* original, const texture_descriptor& desc, status& out_status)
    {
        const status compatibility_status = original->is_view_compatible(desc);
        if (is_status_error(compatibility_status))
        {
            out_status = compatibility_status;
            return;
        }

        num_texture_mipmap_levels = 1 + desc.format.mipmap_levels;
        num_texture_array_layers = desc.format.texture_layers;

        const bool has_msaa = desc.format.msaa != texture_msaa_level::NONE;
        const bool can_type_be_array = desc.format.shape != texture_shape::_3D;
        const bool can_shape_be_multisample = desc.format.shape == texture_shape::_2D || desc.format.shape == texture_shape::_3D;
        const bool is_array = (desc.format.shape != texture_shape::CUBE_MAP && num_texture_array_layers > 1) || num_texture_array_layers > 6;

        gl_texture_target = texture_type_to_gl_target(desc.format.shape, desc.format.msaa != texture_msaa_level::NONE, is_array);
        gl_texture_format = texture_data_type_to_gl_format(desc.format.data_type);

        if (num_texture_mipmap_levels <= 0)
        {
            out_status = {status_type::INVALID, std::format("Texture view '{0}' includes zero mipmap layers", desc.identifier().name)};
            return;
        }

        if (num_texture_array_layers <= 0)
        {
            out_status = {status_type::INVALID, std::format("Texture view '{0}' includes zero texture layers", desc.identifier().name)};
            return;
        }

        if (!can_shape_be_multisample && has_msaa)
        {
            out_status = {status_type::INVALID, std::format("Texture view '{0}' shape cannot have MSAA, but msaa level is not zero", desc.identifier().name)};
            return;
        }

        if (!can_type_be_array && is_array)
        {
            out_status = {status_type::INVALID, std::format("Texture view '{0}' shape cannot be an array texture, but includes too many texture layers", desc.identifier().name)};
            return;
        }

        if (desc.format.shape == texture_shape::CUBE_MAP && num_texture_array_layers % 6 != 0)
        {
            out_status = {status_type::INVALID, std::format("Texture view '{0}' shape is cubemap, but number of texture layers included is not a multiple of 6", desc.identifier().name)};
            return;
        }

        if (!is_sampling_config_valid_for_type(desc.format.data_type, desc.default_sampling_config))
        {
            out_status = {status_type::INVALID, std::format("Provided default sampling config isn't valid for texture view '{0}' (are you trying to use linear filtering on an integer texture?)", desc.identifier().name)};
            return;
        }

        glGenTextures(1, &texture_id);

        if (texture_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, std::format("Creating texture view {0} failed", desc.identifier().name)};
            return;
        }

        glTextureView(texture_id, gl_texture_target, original->texture_id, gl_texture_format, desc.format.view_texture_base_mipmap, desc.format.mipmap_levels, desc.format.view_texture_base_array_index, num_texture_array_layers);

        texture_sampling_conifg sampling_config = desc.default_sampling_config;
        sampling_config.mipmap_max_level = std::min(sampling_config.mipmap_max_level, num_texture_mipmap_levels - 1);
        out_status = set_sampling_config(sampling_config);
    }

    texture_state::~texture_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete texture object");
        glDeleteTextures(1, &texture_id);
    }

    GLenum gl_filter_mode_from_modes(const texture_filtering_mode filter, const texture_filtering_mode mipmap_selection)
    {
        const uint8_t option = static_cast<uint8_t>(filter) + static_cast<uint8_t>(mipmap_selection) * 2;
        switch (option)
        {
            case 0: return GL_NEAREST_MIPMAP_NEAREST; //Nearest nearest
            case 1: return GL_LINEAR_MIPMAP_NEAREST;  //Linear nearest
            case 2: return GL_NEAREST_MIPMAP_LINEAR;  //Nearest linear
            case 3: return GL_LINEAR_MIPMAP_LINEAR;   //Linear linear
            default: return -1;
        }
    }

    inline void set_texture_border_color(const GLuint texture_id, const texture_border_color border_color)
    {
        switch (border_color)
        {
            case texture_border_color::INTEGER_BLACK:
            {
                constexpr std::array col = {0, 0, 0, 1};
                glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
            case texture_border_color::INTEGER_WHITE:
            {
                constexpr std::array col = {1, 1, 1, 1};
                glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
            case texture_border_color::INTEGER_TRANSPARENT:
            {
                constexpr std::array col = {0, 0, 0, 0};
                glTextureParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
            case texture_border_color::FLOAT_BLACK:
            {
                constexpr std::array col = {0.f, 0.f, 0.f, 1.f};
                glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
            case texture_border_color::FLOAT_WHITE:
            {
                constexpr std::array col = {1.f, 1.f, 1.f, 1.f};
                glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
            case texture_border_color::FLOAT_TRANSPARENT:
            {
                constexpr std::array col = {0.f, 0.f, 0.f, 0.f};
                glTextureParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                break;
            }
        }
    }

    GLenum gl_wrapping_mode(const texture_wrapping_mode mode)
    {
        switch (mode)
        {
            case texture_wrapping_mode::CLAMP: return GL_CLAMP;
            case texture_wrapping_mode::REPEAT: return GL_REPEAT;
            case texture_wrapping_mode::MIRROR: return GL_MIRRORED_REPEAT;
            case texture_wrapping_mode::BORDER: return GL_CLAMP_TO_BORDER;
        }
        return -1;
    }

    GLenum gl_swizzle_mode(const texture_swizzle swizzle)
    {
        switch (swizzle)
        {
            case texture_swizzle::RED: return GL_RED;
            case texture_swizzle::GREEN: return GL_GREEN;
            case texture_swizzle::BLUE: return GL_BLUE;
            case texture_swizzle::ALPHA: return GL_ALPHA;
        }
        return -1;
    }

    bool texture_state::is_valid() const
    {
        return texture_id != 0;
    }

    bool is_view_format_compatible(const GLenum source_format, const GLenum view_format)
    {
        const std::array<std::vector<GLenum>, 12> compatible_sets = {
            std::vector<GLenum> {GL_RGBA32F, GL_RGBA32UI, GL_RGBA32I},
            {GL_RGB32F, GL_RGB32UI, GL_RGB32I},
            {GL_RGBA16F, GL_RG32F, GL_RGBA16UI, GL_RG32UI, GL_RGBA16I, GL_RG32I, GL_RGBA16, GL_RGBA16_SNORM},
            {GL_RGB16, GL_RGB16_SNORM, GL_RGB16F, GL_RGB16UI, GL_RGB16I},
            {GL_RG16F, GL_R11F_G11F_B10F, GL_R32F, GL_RGB10_A2UI, GL_RGBA8UI, GL_RG16UI, GL_R32UI, GL_RGBA8I, GL_RG16I, GL_R32I, GL_RGB10_A2, GL_RGBA8, GL_RG16, GL_RGBA8_SNORM, GL_RG16_SNORM, GL_SRGB8_ALPHA8, GL_RGB9_E5},
            {GL_RGB8, GL_RGB8_SNORM, GL_SRGB8, GL_RGB8UI, GL_RGB8I},
            {GL_R16F, GL_RG8UI, GL_R16UI, GL_RG8I, GL_R16I, GL_RG8, GL_R16, GL_RG8_SNORM, GL_R16_SNORM},
            {GL_R8UI, GL_R8I, GL_R8, GL_R8_SNORM},
            {GL_COMPRESSED_RED_RGTC1, GL_COMPRESSED_SIGNED_RED_RGTC1},
            {GL_COMPRESSED_RG_RGTC2, GL_COMPRESSED_SIGNED_RG_RGTC2},
            {GL_COMPRESSED_RGBA_BPTC_UNORM, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM},
            {GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT}
        };

        const auto set_ptr = std::ranges::find_if(compatible_sets, [view_format](const std::vector<GLenum>& set)
        {
            return std::ranges::contains(set, view_format);
        });

        if (set_ptr == compatible_sets.end()) return false;
        return std::ranges::contains(*set_ptr, source_format);
    }

    bool is_view_target_compatible(const GLenum source_target, const GLenum view_target)
    {
        const std::array<std::vector<GLenum>, 11> compatible_sets = {
            std::vector<GLenum> {GL_TEXTURE_1D, GL_TEXTURE_1D_ARRAY},
            {GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY},
            {GL_TEXTURE_3D},
            {GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY},
            {GL_TEXTURE_RECTANGLE},
            {GL_TEXTURE_1D_ARRAY, GL_TEXTURE_1D},
            {GL_TEXTURE_2D_ARRAY, GL_TEXTURE_2D},
            {GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_2D, GL_TEXTURE_2D_ARRAY, GL_TEXTURE_CUBE_MAP},
            {GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE_ARRAY},
            {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE},
        };

        const auto set_ptr = std::ranges::find_if(compatible_sets, [source_target](const std::vector<GLenum>& set)
        {
            return set[0] == source_target; //Target compatibility is not 100% commutable - a 2D texture can view into a cube map, but not vice versa.
        });

        if (set_ptr == compatible_sets.end()) return false;
        return std::ranges::contains(*set_ptr, view_target);
    }

    status texture_state::is_view_compatible(const texture_descriptor& view_descriptor) const
    {
        const uint32_t min_view_mipmap = view_descriptor.format.view_texture_base_mipmap;
        const uint32_t max_view_mipmap = min_view_mipmap + view_descriptor.format.mipmap_levels - 1;

        const uint32_t min_view_layer = view_descriptor.format.view_texture_base_array_index;
        const uint32_t max_view_layer = min_view_mipmap + view_descriptor.format.texture_layers - 1;

        const bool is_array = (view_descriptor.format.shape != texture_shape::CUBE_MAP && view_descriptor.format.texture_layers > 1) || view_descriptor.format.texture_layers > 6;

        if (min_view_mipmap + 1 > num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} mipmap levels, but requesting index {2}", view_descriptor.identifier().name, num_texture_mipmap_levels, min_view_mipmap)};
        if (max_view_mipmap + 1 > num_texture_mipmap_levels) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} mipmap levels, but requesting index {2}", view_descriptor.identifier().name, num_texture_mipmap_levels, max_view_mipmap)};
        if (min_view_layer + 1 > num_texture_array_layers) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} texture layers, but requesting index {2}", view_descriptor.identifier().name, num_texture_array_layers, min_view_layer)};
        if (max_view_layer + 1 > num_texture_array_layers) return {status_type::RANGE_OVERFLOW, std::format("Texture view '{0}' is not compatible with source - source has {1} texture layers, but requesting index {2}", view_descriptor.identifier().name, num_texture_array_layers, max_view_layer)};
        if (!is_view_format_compatible(gl_texture_format, texture_data_type_to_gl_format(view_descriptor.format.data_type))) return {status_type::INVALID, std::format("Texture view '{0}' is not compatible with source - texture data types are not compatible", view_descriptor.identifier().name)};
        if (!is_view_target_compatible(gl_texture_target, texture_type_to_gl_target(view_descriptor.format.shape, view_descriptor.format.msaa != texture_msaa_level::NONE, is_array))) return {status_type::INVALID, std::format("Texture view '{0}' is not compatible with source - texture shapes are not compatible", view_descriptor.identifier().name)};
        return status_type::SUCCESS;
    }

    status texture_state::set_sampling_config(const texture_sampling_conifg& config) const
    {
        glTextureParameteri(texture_id, GL_TEXTURE_MAG_FILTER, gl_filter_mode_from_modes(config.upscale_filter, config.mipmap_filter));
        glTextureParameteri(texture_id, GL_TEXTURE_MIN_FILTER, gl_filter_mode_from_modes(config.downscale_filter, config.mipmap_filter));

        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_S, gl_wrapping_mode(config.wrapping_mode_x));
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_T, gl_wrapping_mode(config.wrapping_mode_y));
        glTextureParameteri(texture_id, GL_TEXTURE_WRAP_R, gl_wrapping_mode(config.wrapping_mode_z));

        set_texture_border_color(texture_id, config.border_color);

        if (GLAD_GL_VERSION_4_6 || GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic)
        {
            //Anisotropy is not *strictly* core, but it's supported practically universally
            glTextureParameterf(texture_id, GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(config.anisotropy_level));
        }

        glTextureParameteri(texture_id, GL_TEXTURE_BASE_LEVEL, config.mipmap_min_level);
        glTextureParameteri(texture_id, GL_TEXTURE_MAX_LEVEL, config.mipmap_max_level);
        glTextureParameterf(texture_id, GL_TEXTURE_LOD_BIAS, config.mipmap_bias);

        glTextureParameteri(texture_id, GL_TEXTURE_SWIZZLE_R, gl_swizzle_mode(config.swizzling.swizzle_red));
        glTextureParameteri(texture_id, GL_TEXTURE_SWIZZLE_G, gl_swizzle_mode(config.swizzling.swizzle_green));
        glTextureParameteri(texture_id, GL_TEXTURE_SWIZZLE_B, gl_swizzle_mode(config.swizzling.swizzle_blue));
        glTextureParameteri(texture_id, GL_TEXTURE_SWIZZLE_A, gl_swizzle_mode(config.swizzling.swizzle_alpha));

        return status_type::SUCCESS;
    }
}
