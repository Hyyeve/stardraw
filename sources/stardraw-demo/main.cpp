#include <array>
#include <filesystem>
#include <fstream>

#include "stardraw/api/render_context.hpp"
#include "stardraw/api/shaders.hpp"
#include "starlib/general/logger.hpp"
#include "starlib/types/gameloop.hpp"
#include "starwin/api/window.hpp"
#include "tracy/Tracy.hpp"

using namespace stardraw;
using namespace starwin;
using namespace starlib;

memory_layout_info uniforms_layout;

shader_stage frag_shader;
shader_stage vert_shader;

std::vector<shader_stage> load_shader()
{
    ///Read in the slang source..
    const std::filesystem::path path = "shader.slang";
    std::ifstream file(path, std::ios::in);
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    shader_module main_module;
    shader_program linked_shader;

    ///Init the internal compiler session & load the source
    status init_status = stardraw::setup_shader_compiler();
    status load_status = stardraw::load_shader_module(buffer.str(), main_module);

    ///Create entry points and link them into a shader program
    shader_entry_point vert_entry_point = {main_module, "vertexMain"};
    shader_entry_point frag_entry_point = {main_module, "fragmentMain"};

    status link_status_vtx = stardraw::link_shader_program(
        {
            vert_entry_point,
            frag_entry_point,
        },
        linked_shader
    );

    ///Get individual stages from the linked program
    status vtx_load_status = stardraw::create_shader_stage(linked_shader, vert_entry_point, graphics_api::GL45, vert_shader);
    status frg_load_status = stardraw::create_shader_stage(linked_shader, frag_entry_point, graphics_api::GL45, frag_shader);

    ///Get the buffer layout of our uniform buffer
    status layout_create = stardraw::determine_shader_buffer_layout(frag_shader, "uniforms", uniforms_layout);

    return {vert_shader, frag_shader};
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
            .gl_debug_context = false,
        }, wind);

    wind->set_title("Meow!");
    wind->set_vsync(false);

    render_context* ctx;
    status ctx_status = render_context::create({
        .api = graphics_api::GL45,
        .gl_loader = wind->gl_get_loader_func(),
    }, ctx);

    const std::vector<shader_stage> shader_stages = load_shader();

    const i64 param_buffer_size = frag_shader.buffer_size("uniforms");

    status object_state_status = ctx->create_objects({
            transfer_buffer("transfer_buff", 500),
            buffer("vertices", 300),
            buffer("param-buffer", param_buffer_size * 2),
            texture("tex", texture_format::create_2d(2, 2), texture_sampling_configs::nearest),
            sampler("tex_sampler", texture_sampling_configs::linear, false),
            vertex_configuration(
                "vertex-spec",
                {
                    {"vertices", vertex_data_type::FLOAT3_F32},
                    {"vertices", vertex_data_type::FLOAT4_F32},
                }
            ),
            shader("shader", shader_stages),
            draw_configuration("draw-spec", "vertex-spec", "shader"),
        }
    );

    status made_commands = ctx->create_command_buffer(
        "main",
        {
            aquire(),
            clear_window(attachment_components::ALL),
            draw(draw_mode::TRIANGLES, 3),
            present(),
        }
    );

    void* uniform_mem = layout_memory(uniforms_layout, &uniforms, sizeof(uniform_block));
    free(uniform_mem);

    std::array<u8, 36> texture_bytes = {
        255, 255, 255, 127,
        127, 127, 127, 255,
        255, 255, 255, 255,
        0, 0, 0, 127,
    };

    status transfer_status = ctx->transfer_buffer_memory_immediate({"vertices", "transfer_buff", 0, sizeof(vertex) * 3}, &triangle);
    status tex_transfer_status = ctx->transfer_texture_memory_immediate({"tex", "transfer_buff", 0, 0, 0, 2, 2}, texture_bytes.data());

    status init_status = ctx->execute_command_buffer({
        configure_blending(blending_configs::ALPHA),
        configure_shader(
            "shader",
            {
                {frag_shader.locate("uniforms"), shader_parameter_value::buffer("param-buffer")},
                {frag_shader.locate("uniforms").index(1), shader_parameter_value::vector(1.0f, 0.0f, 1.0f, 1.0f)},
                {frag_shader.locate("texture"), shader_parameter_value::texture("tex")},
                {frag_shader.locate("texture"), shader_parameter_value::sampler("tex_sampler")}
            }),
        configure_draw("draw-spec"),
    });

    starlib::logger logger;
    for (int i = 0; i < 100; i++)
    {
        logger.log_info({"meow", ""}, "meow!");
    }
    logger.flush_logs();

    starlib::gameloop loop {
        .target_fps = 93,
        .update = [wind](const gameloop::loop_data& data)
        {
            wind->poll();

            if (wind->is_close_requested())
            {
                delete wind;
                data.config->should_exit = true;
            }

            TracyPlot("performance factor", data.performance_factor);
        },
        .render = [wind, ctx](const gameloop::loop_data& data)
        {
            wind->refresh();
            status execute_status = ctx->execute_command_buffer("main");
            TracyPlot("frame time", data.delta_time * 1000);
        }
    };

    loop.run();

    status cleanup_status = stardraw::cleanup_shader_compiler();

    return 0;
}
