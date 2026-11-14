#pragma once
#include "Device.h"
#include <memory>
#include <optional>
#include <vector>

namespace VKFW::vulkancore
{

class SubPass
{
  public:
    SubPass()  = default;
    ~SubPass() = default;

    void addColorAttachmentReference(const VkAttachmentReference &reference) { mColorAttachmentReferences.push_back(reference); }
    void addInputAttachmentReference(const VkAttachmentReference &reference) { mInputAttachmentReferences.push_back(reference); }
    void setDepthStencilAttachmentReference(const VkAttachmentReference &reference) { mDepthStencilAttachmentReference = reference; }
    void setResolveAttachmentReference(const VkAttachmentReference &reference) { mResolveAttachmentReference = reference; }
    void validate() const;

  private:
    friend class RenderPass;
    std::vector<VkAttachmentReference>   mColorAttachmentReferences;
    std::vector<VkAttachmentReference>   mInputAttachmentReferences;
    std::optional<VkAttachmentReference> mDepthStencilAttachmentReference;
    std::optional<VkAttachmentReference> mResolveAttachmentReference;
};

class RenderPass
{
  public:
    explicit RenderPass(const std::shared_ptr<Device> &device);
    ~RenderPass();

    void addAttachmentDescription(const VkAttachmentDescription &description);
    void addSubpass(const SubPass &subpass);
    void addDependency(const VkSubpassDependency &dependency);
    void buildRenderPass();

    VkRenderPass getRenderPass() const { return mRenderPass; }

  private:
    std::shared_ptr<Device> mDevice;
    VkRenderPass            mRenderPass = VK_NULL_HANDLE;

    std::vector<VkAttachmentDescription> mAttachmentDescriptions;
    std::vector<SubPass>                 mSubPasses;
    std::vector<VkSubpassDependency>     mDependencies;
};

} // namespace VKFW::vulkancore
