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

    void submitCommandBuffer(VkQueue queue);
    void waitCommandBuffer(VkQueue queue);

    void draw(uint32_t vertexCount);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

    void bindGraphicPipeline(const std::shared_ptr<Pipeline> &pipeline);
    void bindDescriptorSets(const std::shared_ptr<Pipeline> &pipeline, VkDescriptorSet descriptorSet); 
    
    void bindVertexBuffer(const std::vector<VkBuffer> &buffers, uint32_t binding = 0, std::vector<VkDeviceSize> offsets = {0});
    void bindIndexBuffer(VkBuffer buffer, uint32_t offset = 0, VkIndexType indexType = VK_INDEX_TYPE_UINT32);

    // TODO Part2: void pushConstants(...)
    void copyBufferToBuffer(const VkBuffer &source, const VkBuffer &destination, uint32_t count, const std::vector<VkBufferCopy> &regions);
    void copyBufferToImage(VkBuffer src, VkImage dst, VkImageLayout dstLayout, uint32_t width, uint32_t height);
    // TODO L14: void transferImageLayout(...)

  private:
    VkCommandBuffer              mCommandBuffer = VK_NULL_HANDLE;
    std::shared_ptr<CommandPool> mCommandPool   = nullptr;
    std::shared_ptr<Device>      mDevice        = nullptr;
};

} // namespace VKFW::vulkancore