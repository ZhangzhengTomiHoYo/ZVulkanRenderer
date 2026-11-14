#include "RenderPass.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

void SubPass::validate() const
{
    if (mColorAttachmentReferences.empty())
        throw std::runtime_error("SubPass: Color attachment references are empty");
}

RenderPass::RenderPass(const std::shared_ptr<Device> &device) : mDevice(device) {}

RenderPass::~RenderPass()
{
    if (mRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(mDevice->getDevice(), mRenderPass, nullptr);
    }
}

void RenderPass::addAttachmentDescription(const VkAttachmentDescription &description)
{
    mAttachmentDescriptions.push_back(description);
}

void RenderPass::addSubpass(const SubPass &subpass)
{
    mSubPasses.push_back(subpass);
}

void RenderPass::addDependency(const VkSubpassDependency &dependency)
{
    mDependencies.push_back(dependency);
}

void RenderPass::buildRenderPass()
{
    if (mSubPasses.empty() || mAttachmentDescriptions.empty())
        throw std::runtime_error("RenderPass: subpasses or attachments are empty");
    for (const auto &subpass : mSubPasses)
        subpass.validate();

    std::vector<VkSubpassDescription> subpassDescriptions(mSubPasses.size());

    for (size_t i = 0; i < mSubPasses.size(); i++)
    {
        const auto           &subpass       = mSubPasses[i];
        VkSubpassDescription &description   = subpassDescriptions[i];
        description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;                                                                    // 图形渲染SubPass
        description.inputAttachmentCount    = static_cast<uint32_t>(subpass.mInputAttachmentReferences.size());                                   // 输入附件数量
        description.pInputAttachments       = subpass.mInputAttachmentReferences.empty() ? nullptr : subpass.mInputAttachmentReferences.data();   // 输入附件的数组
        description.colorAttachmentCount    = static_cast<uint32_t>(subpass.mColorAttachmentReferences.size());                                   // 颜色附件数量
        description.pColorAttachments       = subpass.mColorAttachmentReferences.data();                                                          // 颜色附件的数组
        description.pDepthStencilAttachment = subpass.mDepthStencilAttachmentReference ? &(*subpass.mDepthStencilAttachmentReference) : nullptr;  // 深度模板附件
        description.pResolveAttachments     = subpass.mResolveAttachmentReference ? &(*subpass.mResolveAttachmentReference) : nullptr;            // 解析附件
    }

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(mAttachmentDescriptions.size());
    renderPassCreateInfo.pAttachments    = mAttachmentDescriptions.data();
    renderPassCreateInfo.subpassCount    = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassCreateInfo.pSubpasses      = subpassDescriptions.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(mDependencies.size());
    renderPassCreateInfo.pDependencies   = mDependencies.empty() ? nullptr : mDependencies.data();

    VkResult result = vkCreateRenderPass(mDevice->getDevice(), &renderPassCreateInfo, nullptr, &mRenderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }
}

} // namespace VKFW::vulkancore
