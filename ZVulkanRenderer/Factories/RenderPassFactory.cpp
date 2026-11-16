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
} // namespace VKFW::factories