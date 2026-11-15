#pragma once
#include "CommandPool.h"
#include "Device.h"
#include "Image.h"
#include "Sampler.h"
#include <memory>
#include <string>

namespace VKFW::vulkancore
{

class Texture
{
  public:
    // 从文件创建纹理，内部创建 Image 和 Sampler
    static std::shared_ptr<Texture> create(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath);

    // 使用已有的 Image 和 Sampler 创建纹理
    static std::shared_ptr<Texture> createFromImage(const std::shared_ptr<Device> &device, const std::shared_ptr<Image> &image, const std::shared_ptr<Sampler> &sampler = nullptr);

    Texture(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath);

    Texture(const std::shared_ptr<Device> &device, const std::shared_ptr<Image> &image, const std::shared_ptr<Sampler> &sampler = nullptr);

    ~Texture() = default;

    const VkDescriptorImageInfo &getImageInfo() const { return mImageInfo; }
    const std::shared_ptr<Image> &getImage() const { return mImage; }
    const std::shared_ptr<Sampler> &getSampler() const { return mSampler; }

  private:
    std::shared_ptr<Image>   mImage   = nullptr;
    std::shared_ptr<Sampler> mSampler = nullptr;
    std::shared_ptr<Device>  mDevice  = nullptr;

    VkDescriptorImageInfo mImageInfo{};
};

} // namespace VKFW::vulkancore