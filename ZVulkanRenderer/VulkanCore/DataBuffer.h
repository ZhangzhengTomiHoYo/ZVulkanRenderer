#pragma once
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "Device.h"
#include <memory>

namespace VKFW::vulkancore
{

class DataBuffer
{
  public:
    static std::shared_ptr<DataBuffer> createVertexBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData);
    static std::shared_ptr<DataBuffer> createIndexBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData);
    static std::shared_ptr<DataBuffer> createStageBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData = nullptr);

    static std::shared_ptr<DataBuffer> createUniformBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size);



    DataBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~DataBuffer();

    void updateBufferByMap(const void *data, VkDeviceSize size);
    void updateBufferByStage(void *data, VkDeviceSize size);

    VkBuffer getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }
    const VkDescriptorBufferInfo &getBufferInfo() const { return mBufferInfo; }

  private:
    void copyBuffer(const VkBuffer &source, const VkBuffer &destination, VkDeviceSize size);
    static uint32_t findMemoryType(const std::shared_ptr<Device> &device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkBuffer                mBuffer     = VK_NULL_HANDLE;
    VkDeviceMemory          mMemory     = VK_NULL_HANDLE;
    VkDescriptorBufferInfo  mBufferInfo = {VK_NULL_HANDLE};
    std::shared_ptr<Device> mDevice     = nullptr;
};

} // namespace VKFW::vulkancore