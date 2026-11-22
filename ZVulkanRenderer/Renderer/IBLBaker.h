#pragma once

#include "../VulkanCore/CommandPool.h"
#include "../VulkanCore/Device.h"
#include "../VulkanCore/Image.h"
#include "../VulkanCore/Sampler.h"
#include "../VulkanCore/Texture.h"
#include "../VulkanCore/CommandBuffer.h"

#include "Camera.h"
#include "Model.h"
#include "OffscreenRenderTarget.h"
#include "DescriptorManager.h"

#include "../Factories/PipelineFactory.h"
#include "../VulkanCore/Shader.h"

#include <memory>
#include <string>
#include <array>

namespace VKFW::renderer
{

class IBLBaker
{
  public:
    IBLBaker(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool);
    // 将 HDR 全景图转换为六面 CubeMap
    std::shared_ptr<vulkancore::Image> equirectToCubemap(const std::string &hdrPath, uint32_t faceSize, const std::string &vertexShaderFile = "spvs/equirect_to_cube_vert.spv", const std::string &fragmentShaderFile = "spvs/equirect_to_cube_frag.spv");
    std::shared_ptr<vulkancore::Image> bakeIrradiance(const std::shared_ptr<vulkancore::Image> &envCubemap, uint32_t faceSize, const std::string &vertSpv = "spvs/irradiance_convolution_vert.spv", const std::string &fragSpv = "spvs/irradiance_convolution_frag.spv");
    std::shared_ptr<vulkancore::Image> bakePrefilter(const std::shared_ptr<vulkancore::Image> envCubemap, uint32_t baseSize, uint32_t mipLevels = 5, const std::string &vertSpv = "spvs/prefilter_specular_vert.spv", const std::string &fragSpv = "spvs/prefilter_specular_frag.spv");
    // 计算分割求和近似中的 BRDF 查找表，通过一次全屏三角形绘制写入二维图像。
    // 横坐标为 NdotV，纵坐标为粗糙度，红、绿通道分别保存缩放和偏移系数。
    // 直接对 BRDF 表达式做数值积分，不需要输入纹理或描述符集。
    // 返回每通道 32 位浮点的 RGBA 二维图像，布局为着色器只读。
    std::shared_ptr<vulkancore::Image> bakeBRDFLUT(uint32_t size, const std::string &vertSpv = "spvs/brdf_lut_vert.spv", const std::string &fragSpv = "spvs/brdf_lut_frag.spv");
  private:
    void initCaptureCameras();

    std::shared_ptr<vulkancore::Device>      mDevice      = nullptr;
    std::shared_ptr<vulkancore::CommandPool> mCommandPool = nullptr;

    Camera mCaptureCameras[6];
};

} // namespace VKFW::renderer