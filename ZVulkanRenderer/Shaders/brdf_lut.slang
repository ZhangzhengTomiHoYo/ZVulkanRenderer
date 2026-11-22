static const float PI = 3.14159265359;

struct VSOutput {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
}

// 根据 SV_VertexID 生成全屏三角形，与 screen_quad 使用相同方式。
static const float2 positions[3] = {
    float2(-1.0, -1.0),
    float2(3.0, -1.0),
    float2(-1.0, 3.0)
};

[shader("vertex")]
VSOutput vertMain(uint vid: SV_VertexID) {
    VSOutput o;
    float2 pos = positions[vid];
    o.sv_position = float4(pos, 0.0, 1.0);
    o.uv = 0.5 * pos + 0.5;
    return o;
}

float radicalInverseVdC(uint i) {
    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    return float(i) * 2.3283064365386963e-10;
}

float2 hammersleyPoint(uint i, uint n) {
    return float2(float(i) / float(n), radicalInverseVdC(i));
}

float3 importanceSampleGGX(float2 xi, float3 N, float roughness) {
    float r4 = pow(roughness, 4.0);

    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (r4 - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;

    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 X = normalize(cross(up, N));
    float3 Y = normalize(cross(N, X));

    float3 Hworld = X * H.x + Y * H.y + N * H.z;
    return normalize(Hworld);
}

float geometry(float NdotX, float roughness) {
    float k = pow(roughness, 2.0) / 2.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float2 integrateBRDF(float NdotV, float roughness) {
    float3 N = float3(0.0, 0.0, 1.0);
    float3 V = float3(sqrt(1.0 - (NdotV * NdotV)), 0.0, NdotV);

    const uint sampleCount = 1000u;
    float A = 0.0;
    float B = 0.0;

    for (uint i = 0u; i < sampleCount; i++) {
        float2 xi = hammersleyPoint(i, sampleCount);
        float3 H = importanceSampleGGX(xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0);

        if (NdotL > 0.0) {
            float Gv = geometry(NdotV, roughness);
            float Gl = geometry(NdotL, roughness);
            float G = Gv * Gl;
            float Vis = (G * HdotV) / (NdotV * NdotH);
            float f = pow(1.0 - HdotV, 5);
            float scale = (1.0 - f) * Vis;
            float bias = f * Vis;
            A += scale;
            B += bias;
        }
    }

    A /= float(sampleCount);
    B /= float(sampleCount);
    return float2(A, B);
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    float2 brdf = integrateBRDF(input.uv.x, input.uv.y);
    return float4(brdf, 0.0, 0.0);
}