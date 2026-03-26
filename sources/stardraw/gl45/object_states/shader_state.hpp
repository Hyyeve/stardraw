#pragma once
#include <format>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "../common.hpp"
#include "stardraw/api/commands.hpp"
#include "../memory_barrier_controller.hpp"

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

        struct object_binding
        {
            object_identifier identifier;
            bool write_access = false;
            GLbitfield read_barriers = 0;
        };

        explicit shader_state(const shader& desc, status& out_status);
        ~shader_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status make_active() const;
        [[nodiscard]] status dispatch_compute(u32 groups_x, u32 groups_y, u32 groups_z) const;
        [[nodiscard]] status dispatch_compute_indirect(u64 indirect_offset) const;
        [[nodiscard]] status upload_parameter(const shader_parameter& parameter);
        void clear_parameters();

        void flag_barriers(memory_barrier_controller& barrier_controller) const;

        [[nodiscard]] descriptor_type object_type() const override;
        void barrier_objects_if_needed(memory_barrier_controller& barrier_controller) const;

        std::vector<u32> descriptor_set_binding_offsets;
        std::vector<shader_parameter> parameter_store;
        std::unordered_map<u32, object_binding> bound_objects;
        bool has_compute_stage = false;

    private:
        [[nodiscard]] status create_from_stages(const std::vector<shader_stage>& stages);

        [[nodiscard]] status remap_spirv_stages(const std::vector<shader_stage>& stages, std::vector<std::string>& out_sources);

        [[nodiscard]] static status link_shader(const std::vector<GLuint>& stages, GLuint& out_shader_id);
        [[nodiscard]] static status compile_shader_stage(const std::string& source, const GLuint type, GLuint& out_shader_id);

        [[nodiscard]] static status validate_program(const GLuint program);

        [[nodiscard]] static std::string get_shader_log(const GLuint shader);
        [[nodiscard]] static std::string get_program_log(const GLuint program);

        GLuint shader_program_id = 0;
    };
}
