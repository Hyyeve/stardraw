#include <array>
#include <filesystem>
#include <fstream>

#include "stardraw/api/shaders.hpp"
#include "stardraw/api/window.hpp"
using namespace stardraw;

shader_buffer_layout* uniforms_layout;

std::vector<shader_stage> load_shader()
{
    const std::filesystem::path path = "shader.slang";
    std::ifstream file(path, std::ios::in);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    status init_status = stardraw::setup_shader_compiler();
    status load_status = stardraw::load_shader_module("basic", buffer.str());
    status link_status_vtx = stardraw::link_shader("basic_pos_col_vtx", "basic", "vertexMain");
    status link_status_frg = stardraw::link_shader("basic_pos_col_frg", "basic", "fragmentMain");

    shader_program* vtx_shader;
    status vtx_load_status = stardraw::create_shader_program("basic_pos_col_vtx", graphics_api::GL45, &vtx_shader);

    shader_program* frg_shader;
    status frg_load_status = stardraw::create_shader_program("basic_pos_col_frg", graphics_api::GL45, &frg_shader);

    status layout_create = stardraw::create_shader_buffer_layout(frg_shader, "uniforms", &uniforms_layout);

    return {{shader_stage_type::VERTEX, vtx_shader}, {shader_stage_type::FRAGMENT, frg_shader}};
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
    1, 1.0f, 1, 0.5f, 0.2, 0.2, 0.2
};

int main()
{
    window* wind;
    stardraw::status wind_status = window::create({graphics_api::GL45}, &wind);
    wind->set_title("Meow!");

    render_context* ctx = wind->get_render_context();

    status object_state_status = ctx->create_objects({
            buffer_descriptor("vertices", 100),
            buffer_descriptor("uniforms", 100),
            vertex_specification_descriptor(
                "vertex-spec",
                {
                    {"vertices", vertex_data_type::FLOAT3_F32},
                    {"vertices", vertex_data_type::FLOAT4_F32},
                }
            ),
            shader_descriptor("shader", load_shader()),
            shader_specification_descriptor("shader-spec", "shader", {{"uniforms", "uniforms"}}),
            draw_specification_descriptor("draw-spec", "vertex-spec", "shader-spec"),
        }
    );

    status made_commands = ctx->create_command_buffer(
        "main",
        {
            clear_window_command(clear_window_mode::ALL),
            draw_command("draw-spec", draw_mode::TRIANGLES, 3),
        }
    );

    void* uniform_mem = layout_shader_buffer_memory(uniforms_layout, &uniforms, sizeof(uniform_block));

    status init_status = ctx->execute_temp_command_buffer({
        buffer_upload_command("vertices", 0, sizeof(vertex) * 3, &triangle),
        buffer_upload_command("uniforms", 0, sizeof(uniform_block), uniform_mem),
        blending_config_command(blending_configs::ALPHA),
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
