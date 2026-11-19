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

    std::vector<std::shared_ptr<DataBuffer>> mBuffers;  // 一个UBO binding, mBuffers.size=3, mTextures.size=0
                                                        // 3是三缓冲的3
    std::vector<std::shared_ptr<Texture>>    mTextures; // 一个纹理binding, mBuffers.size=0, mTextures.size=3
};

} // namespace VKFW::vulkancore