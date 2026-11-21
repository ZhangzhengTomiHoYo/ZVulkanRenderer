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

[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> uCamera;
[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> uObject;
[[vk::binding(1, 1)]] SamplerCube uEnv;

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float3 dir : TEXCOORD0; // 用立方体的局部位置确定方向，归一化后作为法线。
    [[vk::location(1)]] float3 color : COLOR0; // 原样传递，片元着色器未使用。
    [[vk::location(2)]] float2 uv : TEXCOORD1; // 原样传递，片元着色器未使用。
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput o;
    float4 world_position = mul(uObject.model, float4(input.position, 1.0));
    o.sv_position = mul(uCamera.proj, mul(uCamera.view, world_position));
    o.dir = input.position;
    o.color = input.color;
    o.uv = input.uv;
    return o;
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    float3 N = normalize(input.dir);
    float3 T = float3(0.0, 1.0, 0.0);
    float3 B = normalize(cross(T, N));
    T = cross(N, B);

    float3 irradiance = float3(0.0, 0.0, 0.0);

    float phiStep = 2.0 * PI / 1000.0;
    float thetaStep = 0.5 * PI / 250;

    for (float phi = 0.0; phi < 2.0 * PI; phi += phiStep) {
        for (float theta = 0.0; theta < 0.5 * PI; theta += thetaStep) {
            float3 local = float3(
                sin(theta) * cos(phi),
                sin(theta) * sin(phi),
                cos(theta)
            );
            float3 dir = mul(local, float3x3(T, B, N));
            float3 color = uEnv.Sample(normalize(dir)).rgb;
            irradiance += color * cos(theta) * sin(theta) * thetaStep * phiStep;
        }
    }

    return float4(irradiance, 1.0);
}