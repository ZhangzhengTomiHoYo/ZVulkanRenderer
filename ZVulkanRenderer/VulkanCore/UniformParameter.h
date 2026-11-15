#pragma once
#include "DataBuffer.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class UniformParameter
{
  public:
    size_t             mSize           = 0;
    uint32_t           mBinding        = 0;
    uint32_t           mCount          = 1;
    VkDescriptorType   mDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags mStageFlags     = VK_SHADER_STAGE_ALL;

    std::vector<std::shared_ptr<DataBuffer>> mBuffers;
    // TODO：添加每帧的纹理资源
    // 每帧使用一个纹理对象，每个纹理槽位对应一个 UniformParameter
    // 例如：binding 0 对应相机 UBO，binding 1 对应物体 UBO，binding 2 对应基础色纹理
};

} // namespace VKFW::vulkancore