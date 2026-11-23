#include "Model.h"
#include <stdexcept>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_gltf.h"

namespace VKFW::renderer
{

void Model::loadModel(const std::string &path, const std::shared_ptr<vulkancore::Device> &device)
{
    tinyobj::attrib_t                attributes;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      error, warning;

    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &warning, &error, path.c_str()))
        throw std::runtime_error("failed to load model: " + error);

    mVertexData.clear();
    mIndexData.clear();

    // 用位置、UV、法线索引的组合作为键，合并重复顶点
    // OBJ 分别记录三种索引，绘制时统一为一个顶点索引
    std::unordered_map<std::string, uint32_t> uniqueVertices;

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {

            std::string key = std::to_string(index.vertex_index) + "/" + std::to_string(index.texcoord_index) + "/" + std::to_string(index.normal_index);

            if (uniqueVertices.count(key) == 0)
            {
                StaticMeshVertexData vertex{};

                vertex.mPosition = glm::vec3(attributes.vertices[3 * index.vertex_index + 0], attributes.vertices[3 * index.vertex_index + 1], attributes.vertices[3 * index.vertex_index + 2]);

                vertex.mColor = glm::vec3(1.0f);

                if (index.texcoord_index >= 0)
                {
                    vertex.mUV = glm::vec2(attributes.texcoords[2 * index.texcoord_index + 0], 1.0f - attributes.texcoords[2 * index.texcoord_index + 1]);
                    // 翻转 UV 的 V 分量，匹配当前纹理坐标约定
                }
                else
                {
                    vertex.mUV = glm::vec2(0.0f);
                }

                uniqueVertices[key] = static_cast<uint32_t>(mVertexData.size());
                mVertexData.push_back(vertex);
            }

            mIndexData.push_back(uniqueVertices[key]);
        }
    }

    mVertexBuffer = vulkancore::DataBuffer::createVertexBuffer(device, mVertexData.size() * sizeof(StaticMeshVertexData), mVertexData.data());

    mIndexBuffer = vulkancore::DataBuffer::createIndexBuffer(device, mIndexData.size() * sizeof(uint32_t), mIndexData.data());
}

void Model::loadGltfModel(const std::string &path, const std::shared_ptr<vulkancore::Device> &device) 
{
    tinygltf::Model    gltf;
    tinygltf::TinyGLTF loader;
    std::string        err, warn;

    loader.SetImageLoader([](tinygltf::Image *, const int, std::string *, std::string *, int, int, const unsigned char *, int, void *) { return true; }, nullptr);

    bool ok = (path.size() >= 4 && path.substr(path.size() - 4) == ".glb") ? loader.LoadBinaryFromFile(&gltf, &err, &warn, path) : loader.LoadASCIIFromFile(&gltf, &err, &warn, path);
    if (!ok)
        throw std::runtime_error("failed to load glTF: " + err);

    mPBRVertexData.clear();
    mIndexData.clear();

    for (int meshIdx = 0; meshIdx < (int)gltf.meshes.size(); ++meshIdx)
    {
        const auto &mesh = gltf.meshes[meshIdx];

        // 查找引用该网格的节点，将节点变换应用到顶点数据中。
        glm::mat4 nodeMatrix(1.0f);
        for (const auto &node : gltf.nodes)
        {
            if (node.mesh != meshIdx)
                continue;
            if (!node.matrix.empty())
            {
                nodeMatrix = glm::make_mat4(node.matrix.data());
            }
            else
            {
                if (!node.translation.empty())
                    nodeMatrix = glm::translate(nodeMatrix, glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
                if (!node.rotation.empty())
                {
                    glm::quat q((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
                    nodeMatrix *= glm::mat4_cast(q);
                }
                if (!node.scale.empty())
                    nodeMatrix = glm::scale(nodeMatrix, glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
            }
            break;
        }
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(nodeMatrix)));

        for (const auto &prim : mesh.primitives)
        {
            auto getAttr = [&](const std::string &name) -> int {
                auto it = prim.attributes.find(name);
                return it != prim.attributes.end() ? it->second : -1;
            };
            int posIdx  = getAttr("POSITION");
            int normIdx = getAttr("NORMAL");
            int uvIdx   = getAttr("TEXCOORD_0");
            int tanIdx  = getAttr("TANGENT");
            if (posIdx < 0)
                continue;

            int vertexCount  = (int)gltf.accessors[posIdx].count;
            int vertexOffset = (int)mPBRVertexData.size();

            // 获取访问器第 i 个元素的指定类型指针，并考虑 byteStride 字节步长。
            auto ptr = [&](int accIdx, size_t elemSize, int i) -> const uint8_t * {
                const auto &acc    = gltf.accessors[accIdx];
                const auto &view   = gltf.bufferViews[acc.bufferView];
                size_t      stride = view.byteStride ? view.byteStride : elemSize;
                return gltf.buffers[view.buffer].data.data() + view.byteOffset + acc.byteOffset + i * stride;
            };

            for (int i = 0; i < vertexCount; ++i)
            {
                PBRVertexData v{};

                const float *p = reinterpret_cast<const float *>(ptr(posIdx, sizeof(float) * 3, i));
                v.mPosition    = nodeMatrix * glm::vec4(p[0], p[1], p[2], 1.0f);

                if (normIdx >= 0)
                {
                    const float *n   = reinterpret_cast<const float *>(ptr(normIdx, sizeof(float) * 3, i));
                    glm::vec3    nrm = glm::normalize(normalMatrix * glm::vec3(n[0], n[1], n[2]));
                    v.mNormal        = glm::vec4(nrm, 0.0f);
                }
                if (uvIdx >= 0)
                {
                    const float *uv = reinterpret_cast<const float *>(ptr(uvIdx, sizeof(float) * 2, i));
                    v.mTexcoord     = glm::vec4(uv[0], uv[1], 0.0f, 0.0f);
                }
                if (tanIdx >= 0)
                {
                    const float *t   = reinterpret_cast<const float *>(ptr(tanIdx, sizeof(float) * 4, i));
                    glm::vec3    tan = glm::normalize(glm::mat3(nodeMatrix) * glm::vec3(t[0], t[1], t[2]));
                    v.mTangent       = glm::vec4(tan, t[3]);
                }
                mPBRVertexData.push_back(v);
            }

            // 读取索引。
            std::vector<uint32_t> indices;
            if (prim.indices >= 0)
            {
                const auto    &acc   = gltf.accessors[prim.indices];
                const auto    &view  = gltf.bufferViews[acc.bufferView];
                const uint8_t *data  = gltf.buffers[view.buffer].data.data() + view.byteOffset + acc.byteOffset;
                int            count = (int)acc.count;
                indices.resize(count);
                switch (acc.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    const uint32_t *s = reinterpret_cast<const uint32_t *>(data);
                    for (int i = 0; i < count; ++i)
                        indices[i] = s[i] + vertexOffset;
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    const uint16_t *s = reinterpret_cast<const uint16_t *>(data);
                    for (int i = 0; i < count; ++i)
                        indices[i] = s[i] + vertexOffset;
                    break;
                }
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                    for (int i = 0; i < count; ++i)
                        indices[i] = data[i] + vertexOffset;
                    break;
                }
                }
            }

            // glTF 未提供切线时，使用 Lengyel 方法计算。
            if (tanIdx < 0 && normIdx >= 0 && uvIdx >= 0 && !indices.empty())
            {
                std::vector<glm::vec3> tanAcc(vertexCount, glm::vec3(0.0f));
                for (size_t i = 0; i + 2 < indices.size(); i += 3)
                {
                    int       i0  = (int)indices[i] - vertexOffset;
                    int       i1  = (int)indices[i + 1] - vertexOffset;
                    int       i2  = (int)indices[i + 2] - vertexOffset;
                    auto     &v0  = mPBRVertexData[vertexOffset + i0];
                    auto     &v1  = mPBRVertexData[vertexOffset + i1];
                    auto     &v2  = mPBRVertexData[vertexOffset + i2];
                    glm::vec3 e1  = glm::vec3(v1.mPosition) - glm::vec3(v0.mPosition);
                    glm::vec3 e2  = glm::vec3(v2.mPosition) - glm::vec3(v0.mPosition);
                    float     du1 = v1.mTexcoord.x - v0.mTexcoord.x, dv1 = v1.mTexcoord.y - v0.mTexcoord.y;
                    float     du2 = v2.mTexcoord.x - v0.mTexcoord.x, dv2 = v2.mTexcoord.y - v0.mTexcoord.y;
                    float     denom = du1 * dv2 - du2 * dv1;
                    if (glm::abs(denom) < 1e-8f)
                        continue;
                    glm::vec3 T = (1.0f / denom) * (dv2 * e1 - dv1 * e2);
                    tanAcc[i0] += T;
                    tanAcc[i1] += T;
                    tanAcc[i2] += T;
                }
                for (int i = 0; i < vertexCount; ++i)
                {
                    auto     &v = mPBRVertexData[vertexOffset + i];
                    glm::vec3 N = glm::vec3(v.mNormal);
                    glm::vec3 T = tanAcc[i] - glm::dot(N, tanAcc[i]) * N; // 使用 Gram-Schmidt 方法将切线相对法线正交化。
                    if (glm::length(T) > 1e-8f)
                        T = glm::normalize(T);
                    v.mTangent = glm::vec4(T, 1.0f); // 默认手性标记。
                }
            }

            mIndexData.insert(mIndexData.end(), indices.begin(), indices.end());
        }
    }
    mVertexBuffer = vulkancore::DataBuffer::createVertexBuffer(device, mPBRVertexData.size() * sizeof(PBRVertexData), mPBRVertexData.data());

    mIndexBuffer = vulkancore::DataBuffer::createIndexBuffer(device, mIndexData.size() * sizeof(uint32_t), mIndexData.data());
}

void Model::draw(const std::shared_ptr<vulkancore::CommandBuffer> &commandBuffer)
{
    std::vector<VkBuffer>     buffers = {mVertexBuffer->getBuffer()};
    std::vector<VkDeviceSize> offsets = {0};
    commandBuffer->bindVertexBuffer(buffers, 0, offsets);
    commandBuffer->bindIndexBuffer(mIndexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    commandBuffer->drawIndexed(static_cast<uint32_t>(mIndexData.size()), 1, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> Model::getBindingDescriptions() const
{
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(StaticMeshVertexData);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return {binding};
}

std::vector<VkVertexInputAttributeDescription> Model::getAttributeDescriptions() const
{
    std::vector<VkVertexInputAttributeDescription> attributes(3);

    attributes[0].binding  = 0;
    attributes[0].location = 0;
    attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset   = offsetof(StaticMeshVertexData, mPosition);

    attributes[1].binding  = 0;
    attributes[1].location = 1;
    attributes[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset   = offsetof(StaticMeshVertexData, mColor);

    attributes[2].binding  = 0;
    attributes[2].location = 2;
    attributes[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributes[2].offset   = offsetof(StaticMeshVertexData, mUV);

    return attributes;
}

std::vector<VkVertexInputBindingDescription> Model::getPBRBindingDescriptions() const
{
    VkVertexInputBindingDescription b{};
    b.binding   = 0;
    b.stride    = sizeof(PBRVertexData);
    b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return {b};
}

std::vector<VkVertexInputAttributeDescription> Model::getPBRAttributeDescriptions() const
{
    // 位置、纹理坐标、法线、切线四个顶点属性，
    // 供 PBR 着色器建立 TBN 坐标系，使用法线贴图。
    std::vector<VkVertexInputAttributeDescription> attrs(4);

    attrs[0].binding  = 0;
    attrs[0].location = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[0].offset   = offsetof(PBRVertexData, mPosition);

    attrs[1].binding  = 0;
    attrs[1].location = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[1].offset   = offsetof(PBRVertexData, mTexcoord);

    attrs[2].binding  = 0;
    attrs[2].location = 2;
    attrs[2].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[2].offset   = offsetof(PBRVertexData, mNormal);

    attrs[3].binding  = 0;
    attrs[3].location = 3;
    attrs[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[3].offset   = offsetof(PBRVertexData, mTangent);

    return attrs;
}

} // namespace VKFW::renderer