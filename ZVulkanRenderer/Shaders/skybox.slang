// skybox.slang
// Displays a cubemap as a background. Same vertex path as the baker (the
// cube's local position is the lookup direction), but set 1 is now a
// samplerCube instead of a 2D equirect. Reused by every IBL checkpoint
// (env cubemap, irradiance, prefilter) -- only the cubemap bound to set 1
// changes.
//
// set 0 = camera/global (view/proj only)
// set 1 = object matrix + cubemap to display

// ---- set 0 : global / camera ----
struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4   cameraPos; // xyz = camera world position (set 0, shared layout)
};
struct ObjectUBO {
    float4x4 model;
    float4x4 normalMatrix;
};
[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> uCamera;

// ---- set 1 : object matrix + cubemap to display ----
[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> uObject;
[[vk::binding(1, 1)]] SamplerCube uCubemap;


struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color    : COLOR0;
    [[vk::location(2)]] float2 uv       : TEXCOORD0;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float3 dir : TEXCOORD0; // local cube pos == direction
    [[vk::location(1)]] float3 color : COLOR0;     // passthrough (unused in fs)
    [[vk::location(2)]] float2 uv    : TEXCOORD1;   // passthrough (unused in fs)
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput o;
    o.dir = input.position;
    float4 world = mul(uObject.model, float4(input.position, 1.0));
    o.sv_position = mul(uCamera.proj, mul(uCamera.view, world));
    o.color = input.color; // genuine consume of location 1
    o.uv = input.uv;       // genuine consume of location 2
    return o;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target
{
    float3 direction = normalize(input.dir);
    float3 color = uCubemap.Sample(direction).rgb;
    return float4(color, 1.0);
}
