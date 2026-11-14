#include "Pipeline.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

Pipeline::Pipeline(const std::shared_ptr<Device> &device, const std::shared_ptr<RenderPass> &renderPass) : mDevice(device), mRenderPass(renderPass) {}

Pipeline::~Pipeline()
{
    if (mLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(mDevice->getDevice(), mLayout, nullptr);
    if (mPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(mDevice->getDevice(), mPipeline, nullptr);
}

void Pipeline::setShaderGroup(const std::vector<std::shared_ptr<Shader>> &shaders)
{
    mShaders = shaders;
}

void Pipeline::pushBlendAttachment(const VkPipelineColorBlendAttachmentState &state)
{
    mBlendAttachmentStates.push_back(state);
}

void Pipeline::build()
{
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    for (const auto &shader : mShaders)
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage  = shader->getShaderStage();
        shaderStageInfo.module = shader->getShaderModule();
        shaderStageInfo.pName  = shader->getEntryPoint().c_str();
        stages.push_back(shaderStageInfo);
    }

    mBlendState.attachmentCount = static_cast<uint32_t>(mBlendAttachmentStates.size());
    mBlendState.pAttachments    = mBlendAttachmentStates.data();

    mDynamicState.dynamicStateCount = static_cast<uint32_t>(mDynamicStates.size());
    mDynamicState.pDynamicStates    = mDynamicStates.data();

    if (mLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(mDevice->getDevice(), mLayout, nullptr);
        mLayout = VK_NULL_HANDLE;
    }
    VkResult result = vkCreatePipelineLayout(mDevice->getDevice(), &mPipelineLayoutInfo, nullptr, &mLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount          = static_cast<uint32_t>(stages.size());
    pipelineCreateInfo.pStages             = stages.data();
    pipelineCreateInfo.pVertexInputState   = &mVertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &mAssemblyState;
    pipelineCreateInfo.pViewportState      = &mViewportState;
    pipelineCreateInfo.pRasterizationState = &mRasterState;
    pipelineCreateInfo.pMultisampleState   = &mMultisampleState;
    pipelineCreateInfo.pColorBlendState    = &mBlendState;
    pipelineCreateInfo.pDynamicState       = &mDynamicState;
    pipelineCreateInfo.pDepthStencilState  = &mDepthStencilState;
    pipelineCreateInfo.layout              = mLayout;
    pipelineCreateInfo.renderPass          = mRenderPass->getRenderPass();
    pipelineCreateInfo.subpass             = 0;
    pipelineCreateInfo.basePipelineHandle  = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex   = -1;

    if (mPipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(mDevice->getDevice(), mPipeline, nullptr);
        mPipeline = VK_NULL_HANDLE;
    }
    result = vkCreateGraphicsPipelines(mDevice->getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mPipeline);
    if (result != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline");
}

} // namespace VKFW::vulkancore