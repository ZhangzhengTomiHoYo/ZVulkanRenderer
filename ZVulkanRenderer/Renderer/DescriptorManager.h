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
    void attachLightUBO();
    void attachObjectUBO(); // binding 1：模型矩阵
    void attachTexture(const std::shared_ptr<vulkancore::Texture> &texture);
    void attachPerFrameTextures(const std::vector<std::shared_ptr<vulkancore::Texture>> &perFrameTextures);
    // TODO：绑定纹理和采样器

    void build();

    void updateCameraUBO(const CameraUBO &data, int frameIndex);
    void updateLightUBO(const LightUBO &data, int frameIndex);
    void updateObjectUBO(const ObjectUBO &data, int frameIndex);

    std::shared_ptr<vulkancore::DescriptorSetLayout> getLayout() const { return mLayout; }
    VkDescriptorSet getDescriptorSet(int frameIndex) const { return mDescriptorSet->getDescriptorSet(frameIndex); }

  private:
    std::shared_ptr<vulkancore::Device> mDevice     = nullptr;
    
    std::shared_ptr<vulkancore::DescriptorSetLayout> mLayout        = nullptr;
    std::shared_ptr<vulkancore::DescriptorPool>      mPool          = nullptr;
    std::shared_ptr<vulkancore::DescriptorSet>       mDescriptorSet = nullptr;

    std::vector<std::shared_ptr<vulkancore::DescriptorParameter>> mParams;
    int mCameraParamIndex = -1;  // 用于访问 mParams 找哪种资源->相机UBO
    int mObjectParamIndex = -1;  // 找->物体UBO

    int mLightParamIndex = -1;

    int mFrameCount = 1;         // UBO的第几份Buffer
};

} // namespace VKFW::renderer