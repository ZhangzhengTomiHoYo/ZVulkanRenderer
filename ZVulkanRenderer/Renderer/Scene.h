#pragma once

#include "../VulkanCore/CommandBuffer.h"
#include "../VulkanCore/CommandPool.h"
#include "../VulkanCore/Device.h"
#include "../VulkanCore/Pipeline.h"
#include "../VulkanCore/RenderPass.h"
#include "../VulkanCore/SwapChain.h"
#include "../VulkanCore/Texture.h"
#include "Renderer_Common.h"
#include <memory>
#include <vector>
#include <array>

#include "../Factories/PipelineFactory.h"

#include "Camera.h"
#include "DescriptorManager.h"
#include "RenderObjectNode.h"
#include "OffscreenRenderTarget.h"

namespace VKFW::renderer
{

// 场景管理相机、渲染节点、管线、描述符和已录制的命令缓冲区。
class Scene
{
  public:
    Scene(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool, const std::shared_ptr<vulkancore::SwapChain> &swapChain, const std::shared_ptr<vulkancore::RenderPass> &swapChainRenderPass, const std::shared_ptr<OffscreenRenderTarget> &offscreenRT);
    ~Scene() = default;

    void init();

    // 每帧更新动画、相机和物体 UBO。
    void update(float dt, uint32_t frameIndex);

    // 窗口变化后重建全屏描述符和命令；管线使用动态视口，无需重建。
    void onResize();

    std::shared_ptr<vulkancore::CommandBuffer> getCommandBuffer(uint32_t imageIndex) const { return mCommandBuffers.at(imageIndex); }
    std::shared_ptr<Camera>                    getMainCamera() const { return mMainCamera; }

  private:
    void initStaticResources();   // 模型、纹理和相机，窗口变化时继续使用
    void initResourcesAndObjectNodes(); // 场景 set 0、节点 set 1 和全屏描述符
    void buildPipelines();
    void recordCommandBuffers();

    // 初始化和窗口变化时调用，让描述符指向当前离屏图像。
    void createQuadDescriptors();

  private:
    // 与 VulkanApp 共享设备、交换链等对象。
    std::shared_ptr<vulkancore::Device>      mDevice;
    std::shared_ptr<vulkancore::CommandPool> mCommandPool;
    std::shared_ptr<vulkancore::SwapChain>   mSwapChain;
    std::shared_ptr<vulkancore::RenderPass>  mSwapChainRenderPass;
    std::shared_ptr<OffscreenRenderTarget>   mOffscreenRT;

    // 模型、纹理、相机和动画状态。
    bool                                 mStaticReady = false;
    std::shared_ptr<Camera>              mMainCamera  = nullptr;
    std::shared_ptr<vulkancore::Texture> mCubeTexture = nullptr;
    std::shared_ptr<RenderObjectNode>          mCubeObjectNode    = nullptr;
    float                                mRotation    = 0.0f;

    // 场景共享描述符和全屏绘制描述符。
    std::shared_ptr<DescriptorManager> mGlobalDescriptorManager = nullptr; // set 0
    std::shared_ptr<DescriptorManager> mQuadDescriptorManager   = nullptr; // 全屏绘制

    // 场景管线和全屏管线。
    std::shared_ptr<vulkancore::Pipeline> mScenePipeline = nullptr;
    std::shared_ptr<vulkancore::Pipeline> mQuadPipeline  = nullptr;

    // 每张交换链图像对应一份命令缓冲区。
    std::vector<std::shared_ptr<vulkancore::CommandBuffer>> mCommandBuffers{};
};

} // namespace VKFW::renderer