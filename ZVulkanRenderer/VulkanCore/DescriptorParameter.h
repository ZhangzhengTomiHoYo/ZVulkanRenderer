#pragma once
#include "DataBuffer.h"
#include "Texture.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class DescriptorParameter
{
  public:
    size_t             mSize           = 0;
    uint32_t           mBinding        = 0;
    uint32_t           mCount          = 1;
    VkDescriptorType   mDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags mStageFlags     = VK_SHADER_STAGE_ALL;

    std::vector<std::shared_ptr<DataBuffer>> mBuffers;

    std::vector<std::shared_ptr<Texture>>    mTextures;
};

} // namespace VKFW::vulkancore