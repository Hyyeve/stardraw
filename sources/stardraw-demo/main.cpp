#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "../../generated/win64-debug/_deps/tracy-src/public/tracy/Tracy.hpp"
#include "../../generated/win64-debug/_deps/tracy-src/public/tracy/TracyOpenGL.hpp"
#include "../starwin/api/window.hpp"
#include "stardraw/api/render_context.hpp"
#include "stardraw/api/shaders.hpp"
#include "starlib/general/random.hpp"
using namespace stardraw;
using namespace starwin;

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
    f32 position[3];
    f32 color[4];
};

struct uniform_block
{
    f32 tint[4];
    f32 additive[3];
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
    status wind_status = window::create(
        {
            .api = graphics_api::GL45,
            .transparent_framebuffer = true,
            .gl_debug_context = true,
        }, wind);

    wind->set_title("Meow!");

    render_context* ctx;
    status ctx_status = render_context::create({
        .api = graphics_api::GL45,
        .gl_loader = wind->gl_get_loader_func(),
    }, ctx);

    const std::vector<shader_stage> shader_stages = load_shader();

    const u32 param_buffer_size = frag_shader->buffer_size("structured");

    status object_state_status = ctx->create_objects({
            buffer_descriptor("vertices", 300),
            buffer_descriptor("uniforms", 300),
            buffer_descriptor("param-buffer", param_buffer_size * 2),
            texture_descriptor("tex", texture_format::create_2d(2, 2), texture_sampling_configs::nearest),
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
            clear_window_command(clear_window_mode::ALL, {0., 0., 0., 0.}),
            draw_command(draw_mode::TRIANGLES, 3),
            present_command(),
        }
    );

    void* uniform_mem = layout_shader_buffer_memory(uniforms_layout, &uniforms, sizeof(uniform_block));
    free(uniform_mem);

    std::array<u8, 36> texture_bytes = {
        255, 255, 255, 127,
        127, 127, 127, 255,
        255, 255, 255, 255,
        0, 0, 0, 127,
    };

    status transfer_status = ctx->transfer_buffer_memory_immediate({"vertices", 0, sizeof(vertex) * 3}, &triangle);
    status tex_transfer_status = ctx->transfer_texture_memory_immediate({"tex", 0, 0, 0, 2, 2}, texture_bytes.data());

    status init_status = ctx->execute_temp_command_buffer({
        blending_config_command(blending_configs::ALPHA),
        shader_config_command(
            "shader",
            {
                {frag_shader->locate("structured"), shader_parameter_value::buffer("param-buffer")},
                {frag_shader->locate("structured").index(1), shader_parameter_value::vector(1.0f, 0.0f, 1.0f, 1.0f)},
                {frag_shader->locate("texture"), shader_parameter_value::image("tex")}
            }),
        draw_config_command("draw-spec"),
    });

    while (true)
    {
        wind->refresh();
        wind->poll();

        status execute_status = ctx->execute_command_buffer("main");

        if (wind->is_close_requested())
        {
            delete wind;
            break;
        }
    }

    return 0;
}
