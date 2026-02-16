#include <array>
#include <utility>

#include "stardraw/api/shaders.hpp"
#include "stardraw/api/window.hpp"
#include "stardraw/internal/slang.hpp"
using namespace stardraw;

const std::string SLANG_TXT =
"struct TStruct\n"
"{\n"
"    float2 test_f;\n"
"    float3 test_f2;\n"
"};\n"
" \n"
"struct UniformTypes\n"
"{\n"
"    float4x4 u_view_mat;\n"
"    float4x4 u_projection_mat;\n"
"    TStruct[2] test_a;\n"
"    Texture2D u_tex_n;\n"
"    SamplerState u_tex_ss;\n"
"    float3 u_camera_pos;\n"
"}\n"
" \n"
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
"ConstantBuffer<UniformTypes> uniforms;\n"
"ConstantBuffer<UniformTypes> uniforms2;\n"
" \n"
"[shader(\"vertex\")]\n"
"VertexOut vertexMain(VertexIn data)\n"
"{\n"
"    VertexOut output;\n"
" \n"
"    let view_proj = mul(uniforms.u_view_mat, uniforms.u_projection_mat);\n"
"    let vertex_pos = float4(data.position - uniforms2.u_camera_pos, 1.0);\n"
"\n"
"    output.frag_data.color = data.color;\n"
"    output.position = mul(view_proj, vertex_pos);\n"
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


struct packed
{
    float mat_a[16];
    float mat_b[16];
    float c[2] = {0, 1};
    float d[3] = {2, 3, 4};
    float e[2] = {5, 6};
    float f[3] = {7, 8, 9};
    float g[3] = {10, 11, 12};
};

struct padded
{
    float mat_a[16]{};
    float mat_b[16]{};
    float c[2] = {0, 1};
    float _pad0[2];
    float d[3] = {2, 3, 4};
    float _pad1;
    float e[2] = {5, 6};
    float _pad2[2];
    float f[3] = {7, 8, 9};
    float _pad3;
    float g[3] = {10, 11, 12};
    float _pad4;
};

void shader_test()
{
    stardraw::status init_status = stardraw::setup_shader_compiler();
    std::string slang_source = SLANG_TXT;
    stardraw::status load_status = stardraw::load_shader_module("basic", slang_source);
    stardraw::status link_status_vtx = stardraw::link_shader("basic_pos_col_vtx", "basic", "vertexMain");

    stardraw::shader_program* vtx_shader;
    stardraw::status vtx_load_status = stardraw::create_shader_program("basic_pos_col_vtx", graphics_api::GL45, &vtx_shader);

    stardraw::shader_buffer_layout* uniform_buff_layout;
    stardraw::status buffer_layout_status = stardraw::create_shader_buffer_layout(vtx_shader, "uniforms", &uniform_buff_layout);

    packed test_in;
    padded* test_out = static_cast<padded*>(stardraw::layout_shader_buffer_memory(uniform_buff_layout, &test_in, sizeof(packed)));

    padded x = *test_out;

}

int main()
{
    shader_test();

    window* wind;
    stardraw::status wind_status = window::create({graphics_api::GL45}, &wind);
    wind->set_title("Meow!");

    render_context* ctx = wind->get_render_context();

    status object_state_status = ctx->create_objects({
            buffer_descriptor("vertices", 100),
            vertex_specification_descriptor(
                "vertex-spec",
                {
                    {"vertices", vertex_data_type::FLOAT3_F32},
                    {"vertices", vertex_data_type::FLOAT4_F32},
                }
            )
        }
    );

    status command_buffer_status = ctx->create_command_buffer(
        "clear",
        { clear_window_command(clear_window_mode::ALL) }
    );

    status made_draw_buffer = ctx->create_command_buffer(
        "draw",
        {draw_command("vertex spec", draw_mode::TRIANGLES, 1000) }
    );

    while (true)
    {
        wind->TEMP_UPDATE_WINDOW();
        if (wind->is_close_requested())
        {
            delete wind;
            break;
        }
    }

    return 0;
}



