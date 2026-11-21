struct VertexInput {
    [[vk::location(0)]] float3 pos;
    [[vk::location(1)]] float3 color;
    [[vk::location(2)]] float2 uv;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 color;
    [[vk::location(1)]] float2 uv;
};

struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4 cameraPos;
}

struct ObjectUBO {
    float4x4 model;
    float4x4 normalMatrix;
}

[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> camera;
[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> object;
[[vk::binding(1, 1)]] Sampler2D albedo;

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {
    VertexOutput output;
    float4 worldPos = mul(object.model, float4(input.pos, 1.0));
    float4 viewPos = mul(camera.view, worldPos);
    output.position = mul(camera.proj, viewPos);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
    return albedo.Sample(input.uv);
}