#pragma once
#include <format>
#include <queue>
#include <unordered_map>

#include "../staging_buffer_uploader.hpp"
#include "../types.hpp"
#include "stardraw/api/commands.hpp"

namespace stardraw::gl45
{
    using namespace starlib_stdint;
    class shader_state final : public object_state
    {
    public:
        struct binding_block_location
        {
            GLenum type; //buffer type: SSBO, UBO, etc
            GLuint slot;
        };

        explicit shader_state(const shader_descriptor& desc, status& out_status);
        ~shader_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status make_active() const;
        [[nodiscard]] status upload_parameter(const shader_parameter& parameter);
        void clear_parameters();
        [[nodiscard]] descriptor_type object_type() const override;

        std::vector<u32> descriptor_set_binding_offsets;
        std::vector<shader_parameter> parameter_store;
        std::unordered_map<u32, std::string> bound_objects;
    private:
        [[nodiscard]] status create_from_stages(const std::vector<shader_stage>& stages);

        [[nodiscard]] static GLenum gl_shader_type(shader_stage_type stage);
        [[nodiscard]] status remap_spirv_stages(const std::vector<shader_stage>& stages, std::vector<std::string>& out_sources);

        [[nodiscard]] static status link_shader(const std::vector<GLuint>& stages, GLuint& out_shader_id);
        [[nodiscard]] static status compile_shader_stage(const std::string& source, const GLuint type, GLuint& out_shader_id);

        [[nodiscard]] static status validate_program(const GLuint program);

        [[nodiscard]] static std::string get_shader_log(const GLuint shader);
        [[nodiscard]] static std::string get_program_log(const GLuint program);

        GLuint shader_program_id = 0;
    };
}
