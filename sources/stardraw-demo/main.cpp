#include <array>
#include <filesystem>
#include <fstream>

#include "stardraw/api/shaders.hpp"
#include "stardraw/api/window.hpp"
using namespace stardraw;

shader_buffer_layout* uniforms_layout;
shader_program* frag_shader;
shader_program* vert_shader;

std::vector<shader_stage> load_shader()
{
    const std::filesystem::path path = "shader.slang";
    std::ifstream file(path, std::ios::in);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    status init_status = stardraw::setup_shader_compiler();
    status load_status = stardraw::load_shader_module("main", buffer.str());

    shader_entry_point vert_entry_point = {"main", "vertexMain"};
    shader_entry_point frag_entry_point = {"main", "fragmentMain"};

    status link_status_vtx = stardraw::link_shader_modules(
        "main_linked",
        {
            vert_entry_point,
            frag_entry_point,
        }
    );

    status vtx_load_status = stardraw::create_shader_program("main_linked", vert_entry_point, graphics_api::GL45, &vert_shader);
    status frg_load_status = stardraw::create_shader_program("main_linked", frag_entry_point, graphics_api::GL45, &frag_shader);
    status layout_create = stardraw::create_shader_buffer_layout(frag_shader, "uniforms", &uniforms_layout);

    return {{shader_stage_type::VERTEX, vert_shader}, {shader_stage_type::FRAGMENT, frag_shader}};
}

struct vertex
{
    float position[3];
    float color[4];
};

struct uniform_block
{
    float tint[4];
    float additive[3];
};

std::array triangle = {
    vertex {-1, -1, 0, 1, 0, 0, 1},
    vertex {1, -1, 0, 0, 1, 0, 1},
    vertex {0, 1, 0, 0, 0, 1, 1}
};

uniform_block uniforms = {
    1, 1.0f, 1, 1.0f, 0.5, 0.5, 0.5
};

int main()
{
    window* wind;
    stardraw::status wind_status = window::create({graphics_api::GL45}, &wind);
    wind->set_title("Meow!");

    render_context* ctx = wind->get_render_context();

    const std::vector<shader_stage> shader_stages = load_shader();

    const uint32_t param_buffer_size = frag_shader->buffer_size("structured");

    status object_state_status = ctx->create_objects({
            buffer_descriptor("vertices", 100),
            buffer_descriptor("uniforms", 100),
            buffer_descriptor("param-buffer", param_buffer_size * 2),
            vertex_specification_descriptor(
                "vertex-spec",
                {
                    {"vertices", vertex_data_type::FLOAT3_F32},
                    {"vertices", vertex_data_type::FLOAT4_F32},
                }
            ),
            shader_descriptor("shader", shader_stages),
            draw_specification_descriptor("draw-spec", "vertex-spec", "shader"),
        }
    );

    status made_commands = ctx->create_command_buffer(
        "main",
        {
            clear_window_command(clear_window_mode::ALL),
            draw_command(draw_mode::TRIANGLES, 3),
        }
    );

    void* uniform_mem = layout_shader_buffer_memory(uniforms_layout, &uniforms, sizeof(uniform_block));

    status init_status = ctx->execute_temp_command_buffer({
        buffer_upload_command("vertices", 0, sizeof(vertex) * 3, &triangle),
        blending_config_command(blending_configs::ALPHA),
        shader_config_command(
            "shader",
            {
                {frag_shader->locate("structured"), shader_parameter_value::buffer("param-buffer")},
                {frag_shader->locate("structured").index(1).field("tint"), shader_parameter_value::vector(1.0f, 0.0f, 1.0f, 1.0f)},
            }),
        draw_config_command("draw-spec"),
    });

    free(uniform_mem);

    while (true)
    {
        wind->TEMP_UPDATE_WINDOW();

        status execute_status = ctx->execute_command_buffer("main");


        if (wind->is_close_requested())
        {
            delete wind;
            break;
        }
    }

    return 0;
}
