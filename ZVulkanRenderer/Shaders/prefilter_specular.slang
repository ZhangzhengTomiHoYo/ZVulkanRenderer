static const float PI = 3.14159265359;

// ---- set 0：共享数据 / 相机 ----
struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4   cameraPos;
};
struct ObjectUBO {
    float4x4 model;
    float4x4 normalMatrix;
};
[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> uCamera;

// ---- set 1：物体矩阵 + 输入的环境 CubeMap ----
[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> uObject;
[[vk::binding(1, 1)]] SamplerCube uEnv;

// ---- 推送常量：只传入当前 mip 对应的粗糙度 ----
struct PushConstants {
    float roughness;
};
[[vk::push_constant]] PushConstants pc;

struct VSInput {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 color : COLOR0;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float3 dir : TEXCOORD0; // 用立方体的局部位置确定方向。
    [[vk::location(1)]] float3 color : COLOR0; // 原样传递，片元着色器未使用。
    [[vk::location(2)]] float2 uv : TEXCOORD1; // 原样传递，片元着色器未使用。
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput o;
    o.dir = input.position;
    float4 world = mul(uObject.model, float4(input.position, 1.0));
    o.sv_position = mul(uCamera.proj, mul(uCamera.view, world));
    o.color = input.color;
    o.uv = input.uv;
    return o;
}

// 低差异序列
float radicalInverseVdC(uint i) {
    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    return float(i) * 2.3283064365386963e-10; // / 0x100000000
}
float2 hammersleyPoint(uint i, uint n) {
    return float2(float(i) / float(n), radicalInverseVdC(i));
}

// 重要性采样
float3 importanceSampleGGX(float2 xi, float3 N, float roughtness) {
    float r4 = pow(roughtness, 4.0);

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (r4 - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(0.0, 1.0, 0.0);
    float3 X = normalize(cross(up, N));
    float3 Y = normalize(cross(N, X));

    float3 Hworld = X * H.x + Y * H.y + N * H.z;
    return normalize(Hworld);
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    float3 N = normalize(input.dir);
    float3 V = N;

    float roughness = pc.roughness;

    float3 prefiltered = float3(0.0, 0.0, 0.0);
    float weight = 0.0;
    const uint sampleCount = 1024u;

    for (uint i = 0u; i < sampleCount; i++) {
        float2 xi = hammersleyPoint(i, sampleCount);
        float3 H = importanceSampleGGX(xi, N, roughness);
        float3 L = normalize(2.0 * dot(H, V) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float3 c = uEnv.Sample(L).rgb;
            prefiltered += c * NdotL;
            weight += NdotL;
        }
    }

    prefiltered /= weight;
    return float4(prefiltered, 1.0);
}