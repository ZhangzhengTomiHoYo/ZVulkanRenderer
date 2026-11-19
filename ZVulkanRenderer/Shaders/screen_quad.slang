struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv;
};

[shader("vertex")]
VertexOutput vertMain(uint vertexID: SV_VertexID) {
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(3.0, -1.0),
        float2(-1.0, 3.0)
    };

    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = positions[vertexID] * 0.5 + 0.5;
    return output;
}

[[vk::binding(0, 0)]] Sampler2D sceneTexture;

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    return sceneTexture.Sample(input.uv);
}