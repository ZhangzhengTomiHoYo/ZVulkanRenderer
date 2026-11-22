#include "CommandBuffer.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

CommandBuffer::CommandBuffer(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool) : mDevice(device), mCommandPool(commandPool)
{
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool        = mCommandPool->getCommandPool();
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(mDevice->getDevice(), &allocateInfo, &mCommandBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate command buffer");
}

CommandBuffer::~CommandBuffer()
{
    if (mCommandBuffer != VK_NULL_HANDLE)
        vkFreeCommandBuffers(mDevice->getDevice(), mCommandPool->getCommandPool(), 1, &mCommandBuffer);
}

void CommandBuffer::beginCommandBuffer(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    VkResult result = vkBeginCommandBuffer(mCommandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to begin command buffer");
}

void CommandBuffer::endCommandBuffer()
{
    VkResult result = vkEndCommandBuffer(mCommandBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to end command buffer");
}

void CommandBuffer::beginRenderPass(const VkRenderPassBeginInfo &renderPassBeginInfo, VkSubpassContents contents)
{
    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, contents);
}

void CommandBuffer::endRenderPass()
{
    vkCmdEndRenderPass(mCommandBuffer);
}

void CommandBuffer::bindGraphicPipeline(const std::shared_ptr<Pipeline> &pipeline)
{
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipeline());
}

void CommandBuffer::draw(uint32_t vertexCount)
{
    vkCmdDraw(mCommandBuffer, vertexCount, 1, 0, 0);
}

void CommandBuffer::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) 
{
    vkCmdDrawIndexed(mCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

// 绑定描述符集
void CommandBuffer::bindDescriptorSets(const std::shared_ptr<Pipeline> &pipeline, VkDescriptorSet descriptorSet, uint32_t firstSet) 
{
    vkCmdBindDescriptorSets(
        mCommandBuffer, 
        VK_PIPELINE_BIND_POINT_GRAPHICS, 
        pipeline->getPipelineLayout(), 
        firstSet,        // 从 firstSet 开始
        1,               // 连续绑定 1 个描述符集
        &descriptorSet,  // 绑定 >= 2 个时 此处传数组名字即可
        0, nullptr);
}

void CommandBuffer::bindVertexBuffer(const std::vector<VkBuffer> &buffers, uint32_t binding, std::vector<VkDeviceSize> offsets) 
{
    vkCmdBindVertexBuffers(mCommandBuffer, binding, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets.data());
}

void CommandBuffer::bindIndexBuffer(VkBuffer buffer, uint32_t offset, VkIndexType indexType)
{
    vkCmdBindIndexBuffer(mCommandBuffer, buffer, offset, indexType);
}

void CommandBuffer::copyBufferToBuffer(const VkBuffer &source, const VkBuffer &destination, uint32_t count, const std::vector<VkBufferCopy> &regions)
{
    vkCmdCopyBuffer(mCommandBuffer, source, destination, count, regions.data());
}

void CommandBuffer::copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t width, uint32_t height) 
{
    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {width, height, 1};

    vkCmdCopyBufferToImage(mCommandBuffer, src, dst, dstLayout, 1, &region);
}

void CommandBuffer::copyImageToCubeFace(VkImage source, VkImage destination, uint32_t width, uint32_t height, uint32_t faceIndex, uint32_t mipLevel)
{
    VkImageCopy region{};
    region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel       = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount     = 1;
    region.srcOffset                     = {0, 0, 0};

    region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel       = mipLevel;
    region.dstSubresource.baseArrayLayer = faceIndex;
    region.dstSubresource.layerCount     = 1;
    region.dstOffset                     = {0, 0, 0};

    region.extent = {width, height, 1};

    vkCmdCopyImage(mCommandBuffer, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::pushConstants(VkPipelineLayout layout, VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void *data) 
{
    vkCmdPushConstants(mCommandBuffer, layout, stages, offset, size, data);
}

void CommandBuffer::copyImageToImage(VkImage src, VkImage dst, uint32_t width, uint32_t height, uint32_t dstMipLevel) 
{
    VkImageCopy region{};
    region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.mipLevel       = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount     = 1;
    region.srcOffset                     = {0, 0, 0};

    region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.mipLevel       = dstMipLevel;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount     = 1;
    region.dstOffset                     = {0, 0, 0};

    region.extent = {width, height, 1};

    vkCmdCopyImage(mCommandBuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::submitCommandBuffer(VkQueue queue)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &mCommandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void CommandBuffer::waitCommandBuffer(VkQueue queue)
{
    vkQueueWaitIdle(queue);
}
} // namespace VKFW::vulkancore