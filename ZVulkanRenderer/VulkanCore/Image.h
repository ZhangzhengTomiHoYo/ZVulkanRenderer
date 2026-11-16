#pragma once
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "DataBuffer.h"
#include "Device.h"
#include <cstddef>
#include <memory>
#include <string>

namespace VKFW::vulkancore
{

struct ImageDescription
{
    uint32_t              width       = 0;
    uint32_t              height      = 0;
    uint32_t              depth       = 1;
    uint32_t              mipLevels   = 1;
    uint32_t              arrayLayers = 1;
    VkFormat              format      = VK_FORMAT_UNDEFINED;
    VkImageType           imageType   = VK_IMAGE_TYPE_2D;
    VkImageTiling         tiling      = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags     usage       = 0;
    VkSampleCountFlagBits samples     = VK_SAMPLE_COUNT_1_BIT;
    VkMemoryPropertyFlags properties  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageAspectFlags    aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
};

class Image
{
  public:
    // 从文件读取普通图片，并完成图像上传
    static std::shared_ptr<Image> createImageFromFile(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool flipVertically = false);

    // TODO：创建深度模板图像
    static std::shared_ptr<Image> createDepthStencil(const std::shared_ptr<Device>& device, uint32_t width, uint32_t height, VkFormat depthFormat = VK_FORMAT_UNDEFINED);
    static VkFormat findDepthFormat(const std::shared_ptr<Device> &device, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);


    Image(const std::shared_ptr<Device> &device, const ImageDescription &description);
    ~Image();

    void fillImageData(size_t size, const void *pData, const std::shared_ptr<CommandPool> &commandPool);

    void transitionLayout(VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const std::shared_ptr<CommandPool> &commandPool);

    VkImageView getImageView() const { return mImageView; }
    VkImage getImage() const { return mImage; }
    VkFormat getFormat() const { return mDescription.format; }
    VkImageLayout getLayout() const { return mCurrentLayout; }
    const ImageDescription &getDesc() const { return mDescription; }

  private:
    void createResources();
    void createImageView();
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkAccessFlags getSrcAccessMask(VkImageLayout layout);
    VkAccessFlags getDstAccessMask(VkImageLayout layout);

    std::shared_ptr<Device> mDevice;
    ImageDescription        mDescription;

    VkImage        mImage         = VK_NULL_HANDLE;
    VkDeviceMemory mMemory        = VK_NULL_HANDLE;
    VkImageView    mImageView     = VK_NULL_HANDLE;
    VkImageLayout  mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkExtent3D     mExtent{};
};

} // namespace VKFW::vulkancore