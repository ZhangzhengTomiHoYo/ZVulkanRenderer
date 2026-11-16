#pragma once
#include "../VulkanCore/CommandBuffer.h"
#include "../VulkanCore/DataBuffer.h"
#include "../VulkanCore/Device.h"
#include "Renderer_Common.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace VKFW::renderer
{

struct StaticMeshVertexData
{
    glm::vec3 mPosition;
    glm::vec3 mColor;
    glm::vec2 mUV;
};

class Model
{
  public:
    Model()  = default;
    ~Model() = default;

    void loadModel(const std::string &path, const std::shared_ptr<vulkancore::Device> &device);

    void draw(const std::shared_ptr<vulkancore::CommandBuffer> &commandBuffer);

    std::vector<VkVertexInputBindingDescription> getBindingDescriptions() const;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() const;

    size_t getIndexCount() const { return mIndexData.size(); }

  private:
    std::vector<StaticMeshVertexData> mVertexData;
    std::vector<uint32_t>             mIndexData;

    std::shared_ptr<vulkancore::DataBuffer> mVertexBuffer = nullptr;
    std::shared_ptr<vulkancore::DataBuffer> mIndexBuffer  = nullptr;
};

} // namespace VKFW::renderer