#include "Model.h"
#include <stdexcept>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

} // namespace VKFW::renderer