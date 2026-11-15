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

    static std::shared_ptr<DataBuffer> createStageBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData = nullptr);

    // TODO：创建 Uniform Buffer
    // TODO：创建索引缓冲区

    DataBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~DataBuffer();

    void updateBufferByMap(const void *data, VkDeviceSize size);
    void updateBufferByStage(void *data, VkDeviceSize size);

    VkBuffer getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }

  private:
    void copyBuffer(const VkBuffer &source, const VkBuffer &destination, VkDeviceSize size);
    static uint32_t findMemoryType(const std::shared_ptr<Device> &device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkBuffer                mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory          mMemory = VK_NULL_HANDLE;
    std::shared_ptr<Device> mDevice = nullptr;
};

} // namespace VKFW::vulkancore