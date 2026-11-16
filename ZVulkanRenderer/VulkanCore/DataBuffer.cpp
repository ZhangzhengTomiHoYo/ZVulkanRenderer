#include "DataBuffer.h"
#include <cstring>
#include <stdexcept>

namespace VKFW::vulkancore
{

DataBuffer::DataBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : mDevice(device)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size        = size;
    bufferCreateInfo.usage       = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(mDevice->getDevice(), &bufferCreateInfo, nullptr, &mBuffer);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create buffer");

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(mDevice->getDevice(), mBuffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(mDevice, memoryRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(mDevice->getDevice(), &allocateInfo, nullptr, &mMemory);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to allocate buffer memory");

    vkBindBufferMemory(mDevice->getDevice(), mBuffer, mMemory, 0);

    mBufferInfo.buffer = mBuffer;
    mBufferInfo.offset = 0;
    mBufferInfo.range  = size;
}

DataBuffer::~DataBuffer()
{
    if (mMemory != VK_NULL_HANDLE)
        vkFreeMemory(mDevice->getDevice(), mMemory, nullptr);
    if (mBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(mDevice->getDevice(), mBuffer, nullptr);
}

std::shared_ptr<DataBuffer> DataBuffer::createVertexBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData)
{
    auto buffer = std::make_shared<DataBuffer>(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffer->updateBufferByStage(pData, size);
    return buffer;
}

std::shared_ptr<DataBuffer> DataBuffer::createIndexBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData)
{
    auto buffer = std::make_shared<DataBuffer>(device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffer->updateBufferByStage(pData, size);
    return buffer;
}

std::shared_ptr<DataBuffer> DataBuffer::createStageBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size, void *pData)
{
    auto buffer = std::make_shared<DataBuffer>(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (pData != nullptr)
        buffer->updateBufferByMap(pData, size);
    return buffer;
}

std::shared_ptr<DataBuffer> DataBuffer::createUniformBuffer(const std::shared_ptr<Device> &device, VkDeviceSize size)
{
    auto buffer = std::make_shared<DataBuffer>(device, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    return buffer;
}
void DataBuffer::updateBufferByMap(const void *data, VkDeviceSize size)
{
    void *mappedData = nullptr;                                          // 准备接收CPU可访问的内存地址
    vkMapMemory(mDevice->getDevice(), mMemory, 0, size, 0, &mappedData); // 取得这块内存的CPU地址
    memcpy(mappedData, data, static_cast<size_t>(size));                 // 把CPU原始数据复制进去
    vkUnmapMemory(mDevice->getDevice(), mMemory);                        // 结束映射
}

void DataBuffer::updateBufferByStage(void *data, VkDeviceSize size)
{
    auto stagingBuffer = DataBuffer::createStageBuffer(mDevice, size, data);
    copyBuffer(stagingBuffer->getBuffer(), mBuffer, size);
}

void DataBuffer::copyBuffer(const VkBuffer &source, const VkBuffer &destination, VkDeviceSize size)
{
    // 创建临时 CommandPool 用于本次一次性的数据复制
    auto commandPool   = std::make_shared<CommandPool>(mDevice, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    auto commandBuffer = std::make_shared<CommandBuffer>(mDevice, commandPool);

    commandBuffer->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferCopy region{};
    region.size = size;
    commandBuffer->copyBufferToBuffer(source, destination, 1, {region});

    commandBuffer->endCommandBuffer();
    commandBuffer->submitCommandBuffer(mDevice->getGraphicQueue());
    commandBuffer->waitCommandBuffer(mDevice->getGraphicQueue());
}

uint32_t DataBuffer::findMemoryType(const std::shared_ptr<Device> &device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(device->getPhysicalDevice(), &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw std::runtime_error("failed to find suitable memory type");
}

} // namespace VKFW::vulkancore