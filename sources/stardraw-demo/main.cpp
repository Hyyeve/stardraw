#include <array>
#include <utility>

#include "stardraw/api/shaders.hpp"
#include "stardraw/api/window.hpp"
#include "stardraw/internal/slang.hpp"
using namespace stardraw;

const std::string SLANG_TXT =
    "struct VertexIn\n"
    "{\n"
    "    float3	position : POSITION;\n"
    "    float4	color    : COLOR;\n"
    "};\n"
    " \n"
    "struct FragmentIn\n"
    "{\n"
    "    float4 color;\n"
    "};\n"
    " \n"
    "struct VertexOut\n"
    "{\n"
    "    FragmentIn frag_data : FragmentIn;\n"
    "    float4 position : SV_Position;\n"
    "};\n"
    " \n"
    "struct FragmentOut\n"
    "{\n"
    "    float4 color;\n"
    "};\n"
    " \n"
    "[shader(\"vertex\")]\n"
    "VertexOut vertexMain(VertexIn data)\n"
    "{\n"
    "    VertexOut output;\n"
    " \n"
    "    let vertex_pos = float4(data.position, 1.0);\n"
    "\n"
    "    output.frag_data.color = data.color;\n"
    "    output.position = vertex_pos;\n"
    " \n"
    "    return output;\n"
    "}\n"
    " \n"
    "[shader(\"fragment\")]\n"
    "FragmentOut fragmentMain(FragmentIn data : FragmentIn) : SV_Target\n"
    "{\n"
    "    FragmentOut output;\n"
    "    output.color = data.color;\n"
    "    return output;\n"
    "}";

std::vector<shader_stage> load_shader()
{
    status init_status = stardraw::setup_shader_compiler();
    std::string slang_source = SLANG_TXT;
    status load_status = stardraw::load_shader_module("basic", slang_source);
    status link_status_vtx = stardraw::link_shader("basic_pos_col_vtx", "basic", "vertexMain");
    status link_status_frg = stardraw::link_shader("basic_pos_col_frg", "basic", "fragmentMain");

    shader_program* vtx_shader;
    status vtx_load_status = stardraw::create_shader_program("basic_pos_col_vtx", graphics_api::GL45, &vtx_shader);

    shader_program* frg_shader;
    status frg_load_status = stardraw::create_shader_program("basic_pos_col_frg", graphics_api::GL45, &frg_shader);

    return {{shader_stage_type::VERTEX, vtx_shader}, {shader_stage_type::FRAGMENT, frg_shader}};
}

struct vertex
{
    float position[3];
    float color[4];
};

std::array triangle = {
    vertex {-1, -1, 0, 1, 0, 0, 1},
    vertex {1, -1, 0, 0, 1, 0, 1},
    vertex {0, 1, 0, 0, 0, 1, 1}
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

    status upload_status = ctx->execute_temp_command_buffer({
        buffer_upload_command("vertices", 0, sizeof(vertex) * 3, &triangle),
    });

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
