#pragma once
#include "Device.h"
#include "RenderPass.h"
#include "Shader.h"
#include <memory>
#include <vector>

namespace VKFW::vulkancore
{

class Pipeline
{
  public:
    Pipeline(const std::shared_ptr<Device> &device, const std::shared_ptr<RenderPass> &renderPass);
    ~Pipeline();

    VkPipeline getPipeline() const { return mPipeline; }
    VkPipelineLayout getPipelineLayout() const { return mLayout; }

    void setShaderGroup(const std::vector<std::shared_ptr<Shader>> &shaders);
    void pushBlendAttachment(const VkPipelineColorBlendAttachmentState &state);
    void build();

  public:
    // all pipeline states are public so VulkanApp can configure them directly
    VkPipelineVertexInputStateCreateInfo   mVertexInputState{};
    VkPipelineInputAssemblyStateCreateInfo mAssemblyState{};
    VkPipelineViewportStateCreateInfo      mViewportState{};
    VkPipelineRasterizationStateCreateInfo mRasterState{};
    VkPipelineDynamicStateCreateInfo       mDynamicState{};
    VkPipelineMultisampleStateCreateInfo   mMultisampleState{};
    VkPipelineColorBlendStateCreateInfo    mBlendState{};
    VkPipelineDepthStencilStateCreateInfo  mDepthStencilState{};
    VkPipelineLayoutCreateInfo             mPipelineLayoutInfo{};

    std::vector<VkPipelineColorBlendAttachmentState> mBlendAttachmentStates;
    std::vector<VkDynamicState>                      mDynamicStates;
    // TODO L11: add std::vector<VkVertexInputBindingDescription>
    // TODO L11: add std::vector<VkVertexInputAttributeDescription>
    // TODO L12: add std::vector<VkDescriptorSetLayout> for UBO

  private:
    VkPipeline                           mPipeline   = VK_NULL_HANDLE;
    VkPipelineLayout                     mLayout     = VK_NULL_HANDLE;
    std::shared_ptr<Device>              mDevice     = nullptr;
    std::shared_ptr<RenderPass>          mRenderPass = nullptr;
    std::vector<std::shared_ptr<Shader>> mShaders;
};

} // namespace VKFW::vulkancore