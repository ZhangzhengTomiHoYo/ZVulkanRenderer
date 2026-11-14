#pragma once
#include "Device.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace VKFW::vulkancore
{
class Shader
{
  public:
    Shader(const std::shared_ptr<Device> &device, const std::string &fileName, VkShaderStageFlagBits shaderStage, const std::string &entryPoint);
    ~Shader();

    VkShaderModule getShaderModule() const { return mShaderModule; }
    const std::string &getEntryPoint() const { return mEntryPoint; }
    VkShaderStageFlagBits getShaderStage() const { return mShaderStage; }

  private:
    void createShaderModule(const std::string &fileName);

    // 读取着色器二进制文件并按字节数组返回
    static std::vector<char> readBinary(const std::string &fileName)
    {
        std::ifstream file(fileName.c_str(), std::ios::ate | std::ios::binary | std::ios::in);

        if (!file.is_open())
            throw std::runtime_error("failed to open shader file!");

        const size_t fileSize = static_cast<size_t>(file.tellg());

        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

  private:
    std::shared_ptr<Device> mDevice       = nullptr;
    VkShaderModule          mShaderModule = VK_NULL_HANDLE;
    std::string             mEntryPoint   = "main";
    VkShaderStageFlagBits   mShaderStage  = VK_SHADER_STAGE_VERTEX_BIT;
};
} // namespace VKFW::vulkancore