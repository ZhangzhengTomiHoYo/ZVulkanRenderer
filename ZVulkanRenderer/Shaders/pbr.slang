// PBR 着色器。
// 基于物理的着色：使用 Cook-Torrance 计算直接光，
// 使用 IBL 计算间接光；漫反射读取辐照度图，
// 镜面反射读取预过滤图和 BRDF LUT。按参考 GLSL 实现移植，
// 描述符布局适配本项目的共享 set 0
// 和各物体 set 1 约定。
//
// set 0：相机的观察矩阵、投影矩阵、位置，以及灯光，由所有节点共享。
// set 1：物体 UBO、三张 IBL 图像和五张头盔材质贴图。
// binding 0：物体 UBO，包含模型矩阵和法线矩阵。
// binding 1：辐照度 CubeMap；2：镜面预过滤 CubeMap；3：BRDF LUT。
// binding 4：基础颜色；5：法线；6：金属度粗糙度；7：AO；8：自发光。

static const float PI = 3.14159265359;

// ---- set 0：相机，供顶点阶段和片元阶段使用 ----
struct CameraUBO {
    float4x4 view;
    float4x4 proj;
    float4 cameraPos;
};
[[vk::binding(0, 0)]] ConstantBuffer<CameraUBO> uCamera;

// 方向光，位于共享 set 0、binding 1。
struct LightUBO {
    float4 direction;
    float4 color;
};
[[vk::binding(1, 0)]] ConstantBuffer<LightUBO> uLight;

// ---- set 1：各物体独立的数据 ----
struct ObjectUBO {
    float4x4 model;
    float4x4 normalMatrix;
};

[[vk::binding(0, 1)]] ConstantBuffer<ObjectUBO> uObject;

[[vk::binding(1, 1)]] SamplerCube uIrradiance;
[[vk::binding(2, 1)]] SamplerCube uPrefilter;
[[vk::binding(3, 1)]] Sampler2D uBRDFLUT;

[[vk::binding(4, 1)]] Sampler2D uAlbedo;
[[vk::binding(5, 1)]] Sampler2D uNormal;
[[vk::binding(6, 1)]] Sampler2D uMetalRoughness;
[[vk::binding(7, 1)]] Sampler2D uAO;
[[vk::binding(8, 1)]] Sampler2D uEmissive;

// ---- 顶点阶段 ----
struct VSInput {
    [[vk::location(0)]] float4 position : POSITION;
    [[vk::location(1)]] float4 texcoord : TEXCOORD0;
    [[vk::location(2)]] float4 normal : NORMAL;
    [[vk::location(3)]] float4 tangent : TANGENT;
};

struct VSOutput {
    float4 sv_position : SV_Position;
    [[vk::location(0)]] float4 texcoord : TEXCOORD0;
    [[vk::location(1)]] float4 normalWS : NORMAL;
    [[vk::location(2)]] float4 positionWS : TEXCOORD1;
    [[vk::location(3)]] float4 tangentWS : TANGENT;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput o;

    float3 n = normalize(mul(uObject.normalMatrix, float4(input.normal.xyz, 0.0)).xyz);
    o.normalWS = float4(n, 0.0);

    float3 t = normalize(mul(uObject.normalMatrix, float4(input.tangent.xyz, 0.0)).xyz);
    o.tangentWS = float4(t, input.tangent.w);

    o.texcoord = input.texcoord;

    float4 posWS = mul(uObject.model, float4(input.position.xyz, 1.0));
    o.positionWS = posWS;
    o.sv_position = mul(uCamera.proj, mul(uCamera.view, posWS));
    return o;
}

float3 FresnelSchlick(float3 F0, float3 H, float3 V) {
    float HdotV = dot(H, V);
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5);
}

float3 FresnelSchlickRoughness(float3 F0, float HdotV, float roughness) {
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - HdotV, 5);
}

float3 gammaDecode(float3 srgb) {
    return pow(srgb, float3(2.2));
}

float3 sampleNormalMap(float2 uv, float3 N, float3 T, float tangentW) {
    float3 normalTS = uNormal.Sample(uv).xyz * 2.0 - 1.0; // [0,1] -> [-1,1]

    N = normalize(N);
    T = normalize(T - dot(T, N) * N);
    float3 B = cross(N, T) * tangentW;

    float3x3 TBN = float3x3(T, B, N);
    return normalize(mul(normalTS, TBN));
}

float NormalDistributionGGX(float3 N, float3 H, float roughtness) {
    float r4 = pow(roughtness, 4.0);
    float NdotH = dot(N, H);
    float NdotH2 = NdotH * NdotH;
    float deno = PI * pow(NdotH2 * (r4 - 1.0) + 1.0, 2.0);
    return r4 / deno;
}

float GeometrySchlick(float NdotX, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlick(NdotV, roughness) * GeometrySchlick(NdotL, roughness);
}

[shader("fragment")]
float4 fragMain(VSOutput input) : SV_Target {
    float2 uv = input.texcoord.xy;

    float3 lightColor = uLight.color.rgb;
    float lightIntensity = uLight.color.a;

    float3 N = sampleNormalMap(uv, input.normalWS.xyz, input.tangentWS.xyz, input.tangentWS.w);

    float3 L = normalize(-uLight.direction.xyz);
    float3 V = normalize(uCamera.cameraPos.xyz - input.positionWS.xyz);
    float3 H = normalize(L + V);
    float3 R = normalize(2.0 * dot(N, V) * N - V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // glTF 金属度粗糙度贴图：绿色为粗糙度，蓝色为金属度
    float roughness = uMetalRoughness.Sample(uv).g;
    float metallic = uMetalRoughness.Sample(uv).b;

    float3 albedo = gammaDecode(uAlbedo.Sample(uv).rgb);

    float3 F0 = float3(0.04);
    F0 = lerp(F0, albedo, metallic);

    float3 finalColor = float3(0.0);
    float eps = 0.01;

    // ---- 直接光照，使用 Cook-Torrance 模型 ----
    {
        roughness = max(roughness, eps);
        float3 ks = FresnelSchlick(F0, H, V);
        float D = NormalDistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 specular = ks * D * G / (4.0 * NdotL * NdotV + eps);
        finalColor += specular;

        float3 kd = float3(1.0) - ks;
        kd *= 1.0 - metallic;

        float3 diffuse = kd * albedo / PI;
        finalColor += diffuse;

        finalColor *= NdotL * lightColor * lightIntensity;  // 直接光照的贡献
    }

    // ---- 间接光照，使用 IBL ----
    float3 ambient = float3(0.0);
    {
        float3 ks = FresnelSchlickRoughness(F0, NdotV, roughness);
        float3 kd = float3(1.0) - ks;
        kd *= 1.0 - metallic;

        float3 irradiance = uIrradiance.Sample(N).rgb;
        float3 prefiltered = uPrefilter.SampleLevel(R, roughness * 4.0).rgb;
        float2 brdf = uBRDFLUT.Sample(float2(NdotV, roughness)).rg;

        float3 ambientDiffuse = kd * (irradiance / PI) * albedo;
        float3 ambientSpecular = prefiltered * (F0 * brdf.x + brdf.y);

        float ao = uAO.Sample(uv).r;
        ambient = (ambientDiffuse + ambientSpecular) * ao;
        finalColor += ambient;
    }

    // 叠加自发光
    float3 emissive = uEmissive.Sample(uv).rgb;
    return float4(finalColor + emissive, 1.0);
}