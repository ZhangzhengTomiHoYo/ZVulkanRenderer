#include "Image.h"
#include <cassert>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace VKFW::vulkancore
{

std::shared_ptr<Image> Image::createImageFromFile(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath, VkFormat format, bool flipVertically)
{
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

    stbi_uc *pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0)
        throw std::runtime_error("failed to load image: " + filePath);

    VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4; // RGBA8

    ImageDescription description{};
    description.width       = static_cast<uint32_t>(width);
    description.height      = static_cast<uint32_t>(height);
    description.format      = format;
    description.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    auto image = std::make_shared<Image>(device, description);

    // 切换到传输目标布局，准备接收像素数据
    image->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandPool);

    // 通过暂存缓冲区上传像素数据
    image->fillImageData(static_cast<size_t>(size), pixels, commandPool);

    // 切换到着色器只读布局，供片元着色器采样
    image->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, commandPool);

    stbi_image_free(pixels);
    return image;
}

std::shared_ptr<Image> Image::createHDRImageFromFile(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath)
{
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(0);

    float *pixels = stbi_loadf(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels || width <= 0 || height <= 0)
        throw std::runtime_error("failed to load HDR image: " + filePath);

    VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4 * sizeof(float);

    ImageDescription description{};
    description.width       = static_cast<uint32_t>(width);
    description.height      = static_cast<uint32_t>(height);
    description.format      = VK_FORMAT_R32G32B32A32_SFLOAT;
    description.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    auto image = std::make_shared<Image>(device, description);

    image->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandPool);

    image->fillImageData(static_cast<size_t>(size), pixels, commandPool);

    image->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, commandPool);

    stbi_image_free(pixels);
    return image;
}

std::shared_ptr<Image> Image::createDepthStencil(const std::shared_ptr<Device> &device, uint32_t width, uint32_t height, VkFormat depthFormat, VkSampleCountFlagBits samples)
{
    if (depthFormat == VK_FORMAT_UNDEFINED)
    {
        depthFormat = findDepthFormat(device);
    }

    ImageDescription description{};
    description.width       = width;
    description.height      = height;
    description.format      = depthFormat;
    description.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    description.samples     = samples;

    if (depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || depthFormat == VK_FORMAT_D24_UNORM_S8_UINT)
        description.aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;

    return std::make_shared<Image>(device, description);
}

VkFormat Image::findDepthFormat(const std::shared_ptr<Device> &device, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};

    for (const auto &format : depthFormats)
    {
        VkFormatProperties formatProperties{};
        vkGetPhysicalDeviceFormatProperties(device->getPhysicalDevice(), format, &formatProperties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProperties.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProperties.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported depth format");
}

std::shared_ptr<Image> Image::createMultiSampleImage(const std::shared_ptr<Device> &device, uint32_t width, uint32_t height, VkFormat format)
{
    ImageDescription description{};
    description.width       = width;
    description.height      = height;
    description.format      = format;
    description.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    description.samples     = device->getMaxUsableSampleCount();
    return std::make_shared<Image>(device, description);
}

std::shared_ptr<Image> Image::createRenderTargetImage(const std::shared_ptr<Device> &device, uint32_t width, uint32_t height, VkFormat format)
{
    ImageDescription description{};
    description.width       = width;
    description.height      = height;
    description.format      = format;
    description.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    return std::make_shared<Image>(device, description);
}

std::shared_ptr<Image> Image::createCubeMapImage(const std::shared_ptr<Device> &device, uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels)
{
    ImageDescription description{};
    description.width       = width;
    description.height      = height;
    description.format      = format;
    description.mipLevels   = mipLevels; // 镜面反射预过滤使用多个 mip 层级
    description.arrayLayers = 6;         // 六个图层，分别存放立方体的六个面
    description.isCubeMap   = true;
    description.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    description.aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

    return std::make_shared<Image>(device, description);
}

Image::Image(const std::shared_ptr<Device> &device, const ImageDescription &description) : mDevice(device), mDescription(description)
{
    createResources();
    createImageView();
}

Image::~Image()
{
    if (mImageView != VK_NULL_HANDLE)
        vkDestroyImageView(mDevice->getDevice(), mImageView, nullptr);
    if (mImage != VK_NULL_HANDLE)
        vkDestroyImage(mDevice->getDevice(), mImage, nullptr);
    if (mMemory != VK_NULL_HANDLE)
        vkFreeMemory(mDevice->getDevice(), mMemory, nullptr);
}

void Image::createResources()
{
    mExtent = {mDescription.width, mDescription.height, mDescription.depth};

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType     = mDescription.imageType;
    imageCreateInfo.extent        = {mDescription.width, mDescription.height, mDescription.depth};
    imageCreateInfo.mipLevels     = mDescription.mipLevels;
    imageCreateInfo.arrayLayers   = mDescription.arrayLayers;
    imageCreateInfo.format        = mDescription.format;
    imageCreateInfo.tiling        = mDescription.tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage         = mDescription.usage;
    imageCreateInfo.samples       = mDescription.samples;
    imageCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    if (mDescription.isCubeMap)
        imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkResult result = vkCreateImage(mDevice->getDevice(), &imageCreateInfo, nullptr, &mImage);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create image");

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(mDevice->getDevice(), mImage, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, mDescription.properties);

    result = vkAllocateMemory(mDevice->getDevice(), &allocateInfo, nullptr, &mMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate image memory");

    vkBindImageMemory(mDevice->getDevice(), mImage, mMemory, 0);
}

void Image::createImageView()
{
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image    = mImage;
    imageViewCreateInfo.viewType = mDescription.isCubeMap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    // TODO：立方体贴图使用 VK_IMAGE_VIEW_TYPE_CUBE
    imageViewCreateInfo.format                          = mDescription.format;
    imageViewCreateInfo.components                      = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    imageViewCreateInfo.subresourceRange.aspectMask     = mDescription.aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = mDescription.mipLevels;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = mDescription.arrayLayers;
    // TODO：立方体贴图使用 6 个图层

    VkResult result = vkCreateImageView(mDevice->getDevice(), &imageViewCreateInfo, nullptr, &mImageView);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create image view");
}

void Image::fillImageData(size_t size, const void *pData, const std::shared_ptr<CommandPool> &commandPool)
{
    assert(pData && size > 0);

    auto stagingBuffer = DataBuffer::createStageBuffer(mDevice, size, const_cast<void *>(pData));

    auto commandBuffer = std::make_shared<CommandBuffer>(mDevice, commandPool);
    commandBuffer->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    commandBuffer->copyBufferToImage(stagingBuffer->getBuffer(), mImage, mCurrentLayout, mExtent.width, mExtent.height);
    commandBuffer->endCommandBuffer();
    commandBuffer->submitCommandBuffer(mDevice->getGraphicQueue());
    commandBuffer->waitCommandBuffer(mDevice->getGraphicQueue()); // 等待复制完成后再销毁暂存缓冲区
}

void Image::transitionLayout(VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, const std::shared_ptr<CommandPool> &commandPool)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = mCurrentLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = mImage;
    barrier.subresourceRange.aspectMask     = mDescription.aspectFlags;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mDescription.mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = mDescription.arrayLayers;
    barrier.srcAccessMask                   = getSrcAccessMask(mCurrentLayout);
    barrier.dstAccessMask                   = getDstAccessMask(newLayout);

    auto commandBuffer = std::make_shared<CommandBuffer>(mDevice, commandPool);
    commandBuffer->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkCmdPipelineBarrier(commandBuffer->getCommandBuffer(), srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    commandBuffer->endCommandBuffer();
    commandBuffer->submitCommandBuffer(mDevice->getGraphicQueue());
    vkQueueWaitIdle(mDevice->getGraphicQueue());

    mCurrentLayout = newLayout;
}

VkAccessFlags Image::getSrcAccessMask(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    default:
        return 0;
    }
}

VkAccessFlags Image::getDstAccessMask(VkImageLayout layout)
{
    switch (layout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return VK_ACCESS_TRANSFER_READ_BIT;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return VK_ACCESS_SHADER_READ_BIT;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    default:
        return 0;
    }
}

uint32_t Image::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(mDevice->getPhysicalDevice(), &memoryProperties);
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type");
}

} // namespace VKFW::vulkancore