#pragma once
#include "CommandPool.h"
#include "Device.h"
#include "Pipeline.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class CommandBuffer
{
  public:
    CommandBuffer(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool);
    ~CommandBuffer();

    VkCommandBuffer getCommandBuffer() const { return mCommandBuffer; }

    void beginCommandBuffer(VkCommandBufferUsageFlags flags = 0);
    void endCommandBuffer();

    void beginRenderPass(const VkRenderPassBeginInfo &renderPassBeginInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void endRenderPass();

    void bindGraphicPipeline(const std::shared_ptr<Pipeline> &pipeline);
    void draw(uint32_t vertexCount);

    // TODO L11: void bindVertexBuffer(...)
    // TODO L13: void bindIndexBuffer(...)
    // TODO L12: void bindDescriptorSet(...)
    // TODO Part2: void pushConstants(...)
    // TODO L14: void copyBufferToBuffer(...)
    // TODO L14: void copyBufferToImage(...)
    // TODO L14: void transferImageLayout(...)

  private:
    VkCommandBuffer              mCommandBuffer = VK_NULL_HANDLE;
    std::shared_ptr<CommandPool> mCommandPool   = nullptr;
    std::shared_ptr<Device>      mDevice        = nullptr;
};

} // namespace VKFW::vulkancore