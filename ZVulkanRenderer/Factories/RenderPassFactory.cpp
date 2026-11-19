#include "RenderPassFactory.h"

namespace VKFW::factories
{
std::shared_ptr<vulkancore::RenderPass> RenderPassFactory::CreateSimpleRenderPass(const std::shared_ptr<vulkancore::Device> &device, VkFormat colorFormat)
{
    /*
        RenderPass
        ├─ 附件描述[0]：颜色图像应该怎样使用
        ├─ Subpass[0]
        │  └─ 颜色输出[0]：使用上面的附件[0]
        └─ 依赖[0]：规定外部操作与 Subpass 0 之间的同步
    */

    auto renderPass = std::make_shared<vulkancore::RenderPass>(device);

    VkAttachmentDescription colorAttachDescrip{};
    colorAttachDescrip.format         = colorFormat;
    colorAttachDescrip.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachDescrip.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachDescrip.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachDescrip.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachDescrip.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachDescrip.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;       // RenderPass 可以同一张附件指定 开始 和 结束 时一共两种状态
    colorAttachDescrip.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    renderPass->addAttachmentDescription(colorAttachDescrip);

    VkAttachmentReference colorAttachRef{};
    colorAttachRef.attachment = 0;
    colorAttachRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // SubPass 只能指定这一时刻下 这一张附件 的 一个状态

    // SubPass 加入 颜色附件引用
    vulkancore::SubPass subpass;
    subpass.addColorAttachmentReference(colorAttachRef);

    renderPass->addSubpass(subpass);

    /* ↑
        RenderPass 开始          Subpass 0 写颜色             RenderPass 结束
        UNDEFINED          →    COLOR_ATTACHMENT_OPTIMAL  → PRESENT_SRC_KHR
        initialLayout            layout                       finalLayout
    */

    VkSubpassDependency dep1{};
    dep1.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep1.dstSubpass    = 0;
    dep1.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.srcAccessMask = 0;
    dep1.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    renderPass->addDependency(dep1);

    renderPass->buildRenderPass();

    return renderPass;
}
std::shared_ptr<vulkancore::RenderPass> RenderPassFactory::CreateRenderPassWithDepth(const std::shared_ptr<vulkancore::Device> &device, VkFormat colorFormat, VkFormat depthFormat)
{
    auto renderPass = std::make_shared<vulkancore::RenderPass>(device);

    VkAttachmentDescription color{};
    color.format         = colorFormat;
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    renderPass->addAttachmentDescription(color);

    VkAttachmentDescription depth{};
    depth.format         = depthFormat;
    depth.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    renderPass->addAttachmentDescription(depth);

    // 子通道
    vulkancore::SubPass subpass;
    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    subpass.addColorAttachmentReference(colorRef);

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.setDepthStencilAttachmentReference(depthRef);

    renderPass->addSubpass(subpass);

    VkSubpassDependency dep1{};
    dep1.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep1.dstSubpass    = 0;
    dep1.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep1.srcAccessMask = 0;
    dep1.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep1.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    renderPass->addDependency(dep1);

    renderPass->buildRenderPass();

    return renderPass;
}
std::shared_ptr<vulkancore::RenderPass> RenderPassFactory::CreateRenderPassWithMSAA(const std::shared_ptr<vulkancore::Device> &device, VkFormat colorFormat, VkSampleCountFlagBits msaaSamples, VkFormat depthFormat, VkImageLayout imageFinalLayout)
{
    auto renderPass = std::make_shared<vulkancore::RenderPass>(device);

    VkAttachmentDescription msaaColorAttachment{};
    msaaColorAttachment.format = colorFormat;
    msaaColorAttachment.samples = msaaSamples;
    msaaColorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    msaaColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // 结果已经写入解析附件 此附件数据不需要保留
    msaaColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaaColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaColorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaColorAttachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    renderPass->addAttachmentDescription(msaaColorAttachment);

    VkAttachmentReference msaaColorRef{};
    msaaColorRef.attachment = 0;
    msaaColorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription resolveAttachment{};
    resolveAttachment.format = colorFormat;
    resolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    resolveAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolveAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveAttachment.finalLayout    = imageFinalLayout;

    renderPass->addAttachmentDescription(resolveAttachment);

    VkAttachmentReference resolveRef{};
    resolveRef.attachment = 1;
    resolveRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription msaaDepthAttachment{};
    msaaDepthAttachment.format     = depthFormat;
    msaaDepthAttachment.samples    = msaaSamples;
    msaaDepthAttachment.loadOp     = VK_ATTACHMENT_LOAD_OP_CLEAR;
    msaaDepthAttachment.storeOp    = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaDepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaaDepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaDepthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaDepthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    renderPass->addAttachmentDescription(msaaDepthAttachment);

    VkAttachmentReference msaaDepthRef{};
    msaaDepthRef.attachment = 2;
    msaaDepthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    vulkancore::SubPass subPass{};
    subPass.addColorAttachmentReference(msaaColorRef);
    subPass.setResolveAttachmentReference(resolveRef);
    subPass.setDepthStencilAttachmentReference(msaaDepthRef);

    renderPass->addSubpass(subPass);

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    renderPass->addDependency(dependency);

    renderPass->buildRenderPass();
    return renderPass;
}
} // namespace VKFW::factories