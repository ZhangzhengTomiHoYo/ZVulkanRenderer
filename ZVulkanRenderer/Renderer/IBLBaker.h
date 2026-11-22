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
  private:
    void initCaptureCameras();

    std::shared_ptr<vulkancore::Device>      mDevice      = nullptr;
    std::shared_ptr<vulkancore::CommandPool> mCommandPool = nullptr;

    Camera mCaptureCameras[6];
};

} // namespace VKFW::renderer