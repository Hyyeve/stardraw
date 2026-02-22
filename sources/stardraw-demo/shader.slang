struct UniformData
{
    float4 tint;
    float3 additive;
}

struct ParameterData
{
    float3 tint2;
    Sampler2D tex;
}

struct VertexIn
{
    float3	position : POSITION;
    float4	color    : COLOR;
};

struct FragmentIn
{
    float4 color;
    float2 uv;
};

struct VertexOut
{
    FragmentIn frag_data : FragmentIn;
    float4 position : SV_Position;
};

struct FragmentOut
{
    float4 color;
};

ConstantBuffer<UniformData> uniforms;
ParameterBlock<ParameterData> params;
ParameterBlock<ParameterData> params2;
ParameterBlock<ParameterData> params3;

[shader("vertex")]
VertexOut vertexMain(VertexIn data)
{
    VertexOut output;

    let vertex_pos = float4(data.position, 1.0);

    output.frag_data.color = data.color + params3.tint2.rrgg;
    output.frag_data.uv = vertex_pos.xy;
    output.position = vertex_pos;

    return output;
}

[shader("fragment")]
FragmentOut fragmentMain(FragmentIn data : FragmentIn) : SV_Target
{
    FragmentOut output;
    output.color = data.color * uniforms.tint + float4(uniforms.additive.rgb, 1) * params.tex.Sample(data.uv) * params2.tex.Sample(data.uv);
    return output;
};
