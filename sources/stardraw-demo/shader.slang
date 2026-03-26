struct UniformData
{
    float4 tint;
    float3 additive;
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

StructuredBuffer<UniformData> structured;
Sampler2D texture;

[shader("vertex")]
VertexOut vertexMain(VertexIn data)
{
    VertexOut output;

    let vertex_pos = float4(data.position, 1.0);

    output.frag_data.color = data.color;
    output.frag_data.uv = vertex_pos.xy;
    output.position = vertex_pos;

    return output;
}

[shader("fragment")]
FragmentOut fragmentMain(FragmentIn data : FragmentIn) : SV_Target
{
    FragmentOut output;
    output.color = data.color * structured[1].tint * texture.Sample(data.uv * 20.0f);
    return output;
};
