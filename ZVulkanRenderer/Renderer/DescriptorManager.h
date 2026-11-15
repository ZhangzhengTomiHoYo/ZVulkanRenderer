#pragma once
#include "../VulkanCore/CommandPool.h"
#include "../VulkanCore/DataBuffer.h"
#include "../VulkanCore/DescriptorPool.h"
#include "../VulkanCore/DescriptorSet.h"
#include "../VulkanCore/DescriptorSetLayout.h"
#include "../VulkanCore/Device.h"
#include "Renderer_Common.h"
#include <memory>
#include <vector>

namespace VKFW::renderer
{

class DescriptorManager
{
  public:
    DescriptorManager()  = default;
    ~DescriptorManager() = default;

    void init(const std::shared_ptr<vulkancore::Device> &device, int frameCount);

    void attachCameraUBO(); // binding 0：观察矩阵和投影矩阵
    void attachObjectUBO(); // binding 1：模型矩阵
    void attachTexture(const std::shared_ptr<vulkancore::Texture> &texture);
    // TODO：绑定纹理和采样器

    void build();

    void updateCameraUBO(const CameraUBO &data, int frameIndex);
    void updateObjectUBO(const ObjectUBO &data, int frameIndex);

    std::shared_ptr<vulkancore::DescriptorSetLayout> getLayout() const { return mLayout; }
    VkDescriptorSet getDescriptorSet(int frameIndex) const { return mDescriptorSet->getDescriptorSet(frameIndex); }

  private:
    std::shared_ptr<vulkancore::Device>                        mDevice     = nullptr;
    int                                                        mFrameCount = 1;
    std::vector<std::shared_ptr<vulkancore::DescriptorParameter>> mParams;

    std::shared_ptr<vulkancore::DescriptorSetLayout> mLayout        = nullptr;
    std::shared_ptr<vulkancore::DescriptorPool>      mPool          = nullptr;
    std::shared_ptr<vulkancore::DescriptorSet>       mDescriptorSet = nullptr;
};

} // namespace VKFW::renderer