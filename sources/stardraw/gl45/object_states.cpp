#include "object_states.hpp"

#include <format>

#include "stardraw/internal/internal_types.hpp"
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

        if (staging_buff_pointer != nullptr)
        {
            glUnmapNamedBuffer(staging_buffer_id);
            staging_buff_pointer = nullptr;
        }

        if (staging_buffer_id != 0)
        {
            glDeleteBuffers(1, &staging_buffer_id);
        }

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

    status buffer_state::upload_data_direct(const GLintptr address, const void* const data, const GLsizeiptr bytes)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Direct buffer upload");
        if (data == nullptr) return {status_type::UNEXPECTED_NULL, "Data pointer was null!"};
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

    status buffer_state::upload_data_staged(const GLintptr address, const void* const data, const GLintptr bytes)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Staged buffer upload");
        if (data == nullptr) return {status_type::UNEXPECTED_NULL, "Data pointer was null!"};
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        update_staging_buffer_space(); //Clean up any free blocks ahead of us

        if (bytes > remaining_staging_buffer_space)
        {
            //Not enough space - try wrapping to the start of the buffer and cleaning up more blocks
            current_staging_buff_address = 0;
            remaining_staging_buffer_space = 0;
            update_staging_buffer_space();

            if (bytes > remaining_staging_buffer_space)
            {
                //Still not enough - try and create new staging buffer.
                const status prepared = prepare_staging_buffer(std::min(bytes * 3, main_buffer_size));
                if (is_status_error(prepared)) return prepared;
            }
        }

        GLbyte* upload_buffer_pointer = static_cast<GLbyte*>(staging_buff_pointer);
        const GLbyte* source_pointer = static_cast<const GLbyte*>(data);
        memcpy(upload_buffer_pointer + current_staging_buff_address, source_pointer, bytes);

        const status copy_status = copy_data(staging_buffer_id, current_staging_buff_address, address, bytes);

        upload_chunks.emplace(current_staging_buff_address, bytes, glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
        current_staging_buff_address += bytes;
        remaining_staging_buffer_space -= bytes;

        return copy_status;
    }

    status buffer_state::upload_data_temp_copy(const GLintptr address, const void* const data, const GLintptr bytes) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Temp copy buffer upload");

        if (data == nullptr) return {status_type::UNEXPECTED_NULL, "Data pointer was null!"};
        if (!is_in_buffer_range(address, bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested upload range is out of range in buffer '{0}'", buffer_name)};

        GLuint temp_buffer;
        glCreateBuffers(1, &temp_buffer);
        if (temp_buffer == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create temporary upload destination for buffer '{0}'", buffer_name)};

        glNamedBufferStorage(temp_buffer, bytes, nullptr, GL_MAP_WRITE_BIT);
        GLbyte* pointer = static_cast<GLbyte*>(glMapNamedBuffer(temp_buffer, GL_WRITE_ONLY));
        if (pointer == nullptr)
        {
            glDeleteBuffers(1, &temp_buffer);
            return {status_type::BACKEND_ERROR, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_name)};
        }

        memcpy(pointer, data, bytes);

        const bool unmap_success = glUnmapNamedBuffer(temp_buffer);
        if (!unmap_success)
        {
            glDeleteBuffers(1, &temp_buffer);
            return {status_type::BACKEND_ERROR, std::format("Unable to write to temporary upload destination for buffer '{0}'", buffer_name)};
        }

        const status copy_status = copy_data(temp_buffer, 0, address, bytes);
        glDeleteBuffers(1, &temp_buffer);

        return copy_status;
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

    status buffer_state::prepare_staging_buffer(const GLsizeiptr size)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Allocate staging buffer");

        if (staging_buff_pointer != nullptr)
        {
            glUnmapNamedBuffer(staging_buffer_id);
            staging_buff_pointer = nullptr;
        }

        if (staging_buffer_id != 0)
        {
            glDeleteBuffers(1, &staging_buffer_id);
        }

        while (!upload_chunks.empty())
        {
            const upload_chunk& chunk = upload_chunks.front();
            upload_chunks.pop();
            glDeleteSync(chunk.fence);
        }

        staging_buffer_size = size;
        current_staging_buff_address = 0;
        remaining_staging_buffer_space = size;

        glCreateBuffers(1, &staging_buffer_id);
        if (staging_buffer_id == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create staging buffer for buffer '{0}'", buffer_name)};

        constexpr GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        glNamedBufferStorage(staging_buffer_id, staging_buffer_size, nullptr, flags);

        staging_buff_pointer = glMapNamedBufferRange(staging_buffer_id, 0, staging_buffer_size, flags);
        if (staging_buffer_id == 0) return {status_type::BACKEND_ERROR, std::format("Unable to create staging buffer for buffer '{0}'", buffer_name)};

        return status_type::SUCCESS;
    }

    void buffer_state::update_staging_buffer_space()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Staging buffer free space check");

        while (true)
        {
            if (upload_chunks.empty()) break;

            const upload_chunk& previous_chunk = upload_chunks.front();
            if (previous_chunk.address < current_staging_buff_address) break;

            const GLenum status = glClientWaitSync(previous_chunk.fence, 0, 0);
            if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED)
            {
                upload_chunks.pop();
                remaining_staging_buffer_space += previous_chunk.size;
            }
            else
            {
                break;
            }
        }
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

    status shader_state::get_binding_slot(const std::string_view& name, binding_block_location& out_location) const
    {
        if (!binding_block_locations.contains(std::string(name))) return {status_type::UNKNOWN_NAME, std::format("Shader does not contain a binding called '{0}'", name)};
        out_location = binding_block_locations.at(std::string(name));
        return status_type::SUCCESS;
    }

    status shader_state::write_parameter(const shader_parameter& parameter) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Write shader parameter")
        const shader_parameter_value& value = parameter.value;
        const shader_parameter_location& location = parameter.location;

        return status_type::SUCCESS;
    }

    descriptor_type shader_state::object_type() const
    {
        return descriptor_type::SHADER;
    }

    status shader_state::create_from_stages(const std::vector<shader_stage>& stages)
    {
        std::vector<GLuint> shader_stages;
        status stages_compile_status = status_type::SUCCESS;
        for (const shader_stage& stage : stages)
        {
            const GLenum shader_type = gl_shader_type(stage.type);
            if (shader_type == 0)
            {
                stages_compile_status = {status_type::BACKEND_ERROR, "A provided shader stage is not supported on this API!"};
                break;
            }

            const std::string source = std::string(std::string_view(static_cast<const char*>(stage.program->data), stage.program->data_size));
            GLuint compiled_stage;
            const status compile_status = compile_shader_stage(source, shader_type, compiled_stage);
            if (is_status_error(compile_status))
            {
                stages_compile_status = compile_status;
                break;
            }

            const status load_interface_status = load_interface_locations(stage);
            if (is_status_error(load_interface_status))
            {
                stages_compile_status = load_interface_status;
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

    [[nodiscard]] status shader_state::add_binding_block(const std::string& name, const GLenum binding_type, const uint32_t slot)
    {
        if (binding_block_locations.contains(name))
        {
            const binding_block_location& existing = binding_block_locations[name];
            if (existing.type != binding_type || existing.slot != slot)
            {
                return {status_type::DUPLICATE_NAME, std::format("Aliasing binding block in shader! More than one block named '{0}'", name)};
            }
            return status_type::SUCCESS;
        }

        binding_block_locations[std::string(name)] = {binding_type, slot};
        return status_type::SUCCESS;
    }

    status shader_state::add_parameter_block(const std::string& name)
    {
        parameter_blocks[name] = parameter_block_info {};
        return status_type::SUCCESS;
    }

    status shader_state::load_interface_locations(const shader_stage& stage)
    {
        slang::ShaderReflection* reflection = shader_reflection_of(stage.program);
        slang::TypeLayoutReflection* globals = reflection->getGlobalParamsTypeLayout();
        for (uint32_t idx = 0; idx < globals->getFieldCount(); idx++)
        {
            slang::VariableLayoutReflection* root_param = globals->getFieldByIndex(idx);
            const uint32_t slot = globals->getFieldBindingRangeOffset(idx);
            const std::string name = root_param->getName();
            const slang::TypeReflection::Kind kind = root_param->getType()->getKind();

            switch (kind)
            {
                case slang::TypeReflection::Kind::ConstantBuffer:
                {
                    const status add_status = add_binding_block(name, GL_UNIFORM_BUFFER, slot);
                    if (is_status_error(add_status)) return add_status;
                    continue;
                }
                case slang::TypeReflection::Kind::ShaderStorageBuffer:
                {
                    const status add_status = add_binding_block(name, GL_SHADER_STORAGE_BUFFER, slot);
                    if (is_status_error(add_status)) return add_status;
                    continue;
                }
                case slang::TypeReflection::Kind::ParameterBlock:
                {
                    const status add_status = add_parameter_block(name);
                    if (is_status_error(add_status)) return add_status;
                    continue;
                }
                default: continue;
            }
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

    draw_specification_state::draw_specification_state(const draw_specification_descriptor& descriptor) : shader_specification(descriptor.shader_specification), vertex_specification(descriptor.vertex_specification) {}

    descriptor_type draw_specification_state::object_type() const
    {
        return descriptor_type::DRAW_SPECIFICATION;
    }
}
