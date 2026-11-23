#pragma once
#include "../VulkanCore/CommandBuffer.h"
#include "../VulkanCore/DataBuffer.h"
#include "../VulkanCore/Device.h"
#include "Renderer_Common.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>


namespace VKFW::renderer
{

struct StaticMeshVertexData
{
    glm::vec3 mPosition;
    glm::vec3 mColor;
    glm::vec2 mUV;
};

struct PBRVertexData
{
    glm::vec4 mPosition;
    glm::vec4 mTexcoord;
    glm::vec4 mNormal;
    glm::vec4 mTangent;
};

class Model
{
  public:
    Model()  = default;
    ~Model() = default;

    void loadModel(const std::string &path, const std::shared_ptr<vulkancore::Device> &device);

    void loadGltfModel(const std::string &path, const std::shared_ptr<vulkancore::Device> &device);

    void draw(const std::shared_ptr<vulkancore::CommandBuffer> &commandBuffer);

    std::vector<VkVertexInputBindingDescription> getBindingDescriptions() const;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() const;

    // PBR 顶点格式：location 0～3 分别对应四个 vec4 属性。
    std::vector<VkVertexInputBindingDescription>   getPBRBindingDescriptions() const;
    std::vector<VkVertexInputAttributeDescription> getPBRAttributeDescriptions() const;

    size_t getIndexCount() const { return mIndexData.size(); }

  private:
    std::vector<StaticMeshVertexData> mVertexData;
    std::vector<PBRVertexData>        mPBRVertexData;
    std::vector<uint32_t>             mIndexData;

    std::shared_ptr<vulkancore::DataBuffer> mVertexBuffer = nullptr;
    std::shared_ptr<vulkancore::DataBuffer> mIndexBuffer  = nullptr;
};

} // namespace VKFW::renderer