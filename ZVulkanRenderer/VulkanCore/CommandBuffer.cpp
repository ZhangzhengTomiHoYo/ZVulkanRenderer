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

void CommandBuffer::copyBufferToBuffer(const VkBuffer &source, const VkBuffer &destination, uint32_t count, const std::vector<VkBufferCopy> &regions)
{
    vkCmdCopyBuffer(mCommandBuffer, source, destination, count, regions.data());
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