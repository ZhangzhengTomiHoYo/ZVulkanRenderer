static const float PI = 3.14159265359;

struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4 cameraPos;
};

struct ObjectUBO {
    float4x4 model;
    float4x4 normalMatrix;
};

[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> camera;
[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> object;
[[vk::binding(1, 1)]] Sampler2D equirectTexture;

struct VertexInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct VertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 direction : TEXCOORD0;
    [[vk::location(1)]] float3 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD1;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{
    VertexOutput output;

    output.direction = input.position;

    float4 worldPosition = mul(object.model, float4(input.position, 1.0));
    output.position = mul(camera.proj, mul(camera.view, worldPosition));

    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float2 directionToUV(float3 direction)
{
    float u = atan2(direction.z, direction.x) / (2.0 * PI) + 0.5;
    float v = asin(direction.y) / PI + 0.5;

    return float2(u, v);
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    float3 direction = normalize(input.direction);
    float2 uv = directionToUV(direction);
    float3 hdrColor = equirectTexture.Sample(uv).rgb;

    return float4(hdrColor, 1.0);
}