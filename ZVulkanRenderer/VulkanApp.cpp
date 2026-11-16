#include "VulkanApp.h"
#include <spdlog/spdlog.h>

namespace VKFW
{
void VulkanApp::run()
{
    initWindow();
    initVulkan();
    createSyncObjectes();
    mainLoop();
    cleanUp();
}

void VulkanApp::initWindow()
{
    mWindow = std::make_shared<platform::Window>(mWidth, mHeight);
    mWindow->setOnResize([this](int w, int h) { this->onResize(w, h); });
}

void VulkanApp::initVulkan()
{
    mInstance    = std::make_shared<vulkancore::Instance>(true);
    mSurface     = std::make_shared<vulkancore::WindowSurface>(mInstance, mWindow);
    mDevice      = std::make_shared<vulkancore::Device>(mInstance, mSurface);
    mCommandPool = std::make_shared<vulkancore::CommandPool>(mDevice);
    mSwapchain   = std::make_shared<vulkancore::SwapChain>(mDevice, mWindow, mSurface, mCommandPool);

    spdlog::info("Instance + Surface + Device + CommandPool + Swapchain ready");

    mRenderPass = factories::RenderPassFactory::CreateRenderPassWithDepth(mDevice, mSwapchain->getSwapChainImageFormat());

    mModel = std::make_shared<renderer::Model>();
    mModel->loadModel("../../../Assets/bunny.obj", mDevice);

    // === Framebuffer 创建 ===
    // Swapchain RenderPass Framebuffer 三者密不可分 (离屏渲染除外)
    // Framebuffer的创建依赖RenderPass 并绑定Swapchain的Imageview
    // 本项目Framebuffer封装在Swapchain中
    mSwapchain->createFrameBuffers(mRenderPass);

    mDescriptorManager = std::make_shared<renderer::DescriptorManager>();
    mDescriptorManager->init(mDevice, mSwapchain->getImageCount());
    mDescriptorManager->attachCameraUBO();
    mDescriptorManager->attachObjectUBO();
    mDescriptorManager->build();
    
    mPipeline = createPipeline();
    recordCommandBuffers();
}

void VulkanApp::mainLoop()
{
    while (!mWindow->shouldClose())
    {
        mWindow->pollEvents();
        
        float dt = getFrameTime();
        updateUBO(dt);
        drawFrame();
    }
    vkDeviceWaitIdle(mDevice->getDevice());
}

void VulkanApp::cleanUp()
{
    mCommandBuffers.clear();
    mPipeline.reset();
    mRenderPass.reset();
    mSwapchain.reset();
    mCommandPool.reset();
    mDevice.reset();
    mSurface.reset();
    mInstance.reset();
    mWindow.reset();
}

void VulkanApp::onResize(int w, int h)
{
    mWidth              = w;
    mHeight             = h;
    mFramebufferResized = true;
    spdlog::info("resized: {}x{}", w, h);
}

void VulkanApp::recreateSwapchain() 
{
    mSwapchain->recreateSwapchain();
    mSwapchain->createFrameBuffers(mRenderPass);

    mCommandBuffers.clear();
    recordCommandBuffers();

    createSyncObjectes();
    mCurrentFrame = 0;
}

void VulkanApp::createSyncObjectes()
{
    const uint32_t count = mSwapchain->getImageCount();
    mImageAvailableSemaphores.clear();
    mRenderFinishedSemaphores.clear();
    mFences.clear();

    for (uint32_t i = 0; i < count; i++)
    {
        mImageAvailableSemaphores.push_back(std::make_shared<vulkancore::Semaphore>(mDevice));
        mRenderFinishedSemaphores.push_back(std::make_shared<vulkancore::Semaphore>(mDevice));
        mFences.push_back(std::make_shared<vulkancore::Fence>(mDevice, true)); // Fence初始化为Signaled
    }
}

std::shared_ptr<vulkancore::Pipeline> VulkanApp::createPipeline()
{
    auto pipeline = std::make_shared<vulkancore::Pipeline>(mDevice, mRenderPass);

    // === 着色器 ===
    // SPIR-V 文件由 CMake 编译，并复制到可执行文件旁的 spvs/ 目录
    // 入口名为 "main"：Slang 将 vertMain/fragMain 在 SPIR-V 中的入口名改为 main
    auto vertShader = std::make_shared<vulkancore::Shader>(mDevice, "spvs/hello_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
    auto fragShader = std::make_shared<vulkancore::Shader>(mDevice, "spvs/hello_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
    pipeline->setShaderGroup({vertShader, fragShader});

    // === 顶点输入 ===
    // TODO：添加顶点绑定描述和顶点属性描述
    static std::vector<VkVertexInputBindingDescription>   bindings   = mModel->getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription> attributes = mModel->getAttributeDescriptions();
    
    pipeline->mVertexInputState.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline->mVertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    pipeline->mVertexInputState.pVertexBindingDescriptions      = bindings.data();
    pipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    pipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

    // === 输入装配：每三个顶点组成一个三角形 ===
    pipeline->mAssemblyState.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline->mAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline->mAssemblyState.primitiveRestartEnable = VK_FALSE;

    // === 动态状态与视口 ===
    // 视口和裁剪区域使用动态状态，在命令缓冲区中为每帧设置
    // 不将具体分辨率固定在管线中
    pipeline->mDynamicStates                  = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipeline->mDynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline->mDynamicState.dynamicStateCount = static_cast<uint32_t>(pipeline->mDynamicStates.size());
    pipeline->mDynamicState.pDynamicStates    = pipeline->mDynamicStates.data();

    // 视口和裁剪区域各一个，具体参数在命令缓冲区中动态设置
    pipeline->mViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline->mViewportState.viewportCount = 1;
    pipeline->mViewportState.scissorCount  = 1;

    // === 光栅化 ===
    pipeline->mRasterState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline->mRasterState.depthClampEnable        = VK_FALSE;
    pipeline->mRasterState.rasterizerDiscardEnable = VK_FALSE;
    pipeline->mRasterState.polygonMode             = VK_POLYGON_MODE_FILL;
    pipeline->mRasterState.lineWidth               = 1.0f;
    pipeline->mRasterState.cullMode                = VK_CULL_MODE_BACK_BIT;
    pipeline->mRasterState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline->mRasterState.depthBiasEnable         = VK_FALSE;

    // === 多重采样：当前关闭，后续启用 MSAA ===
    pipeline->mMultisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline->mMultisampleState.sampleShadingEnable  = VK_FALSE;
    pipeline->mMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // === 颜色混合：不混合，直接用输出颜色覆盖 ===
    // TODO：为透明材质启用颜色混合
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable    = VK_FALSE;
    pipeline->pushBlendAttachment(blendAttachment);

    pipeline->mBlendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline->mBlendState.logicOpEnable   = VK_FALSE;
    pipeline->mBlendState.attachmentCount = static_cast<uint32_t>(pipeline->mBlendAttachmentStates.size());
    pipeline->mBlendState.pAttachments    = pipeline->mBlendAttachmentStates.data();

    // === 深度与模板：当前关闭，尚未使用深度缓冲区 ===
    // TODO：启用深度测试和深度写入
    pipeline->mDepthStencilState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline->mDepthStencilState.depthTestEnable  = VK_TRUE;
    pipeline->mDepthStencilState.depthWriteEnable = VK_TRUE;
    pipeline->mDepthStencilState.depthCompareOp   = VK_COMPARE_OP_LESS;

    // === 管线布局：暂不使用描述符集和推送常量 ===
    // TODO：为 UBO 添加描述符集布局
    static VkDescriptorSetLayout descLayouts[] = { 
        mDescriptorManager->getLayout()->getLayout()
    };

    pipeline->mPipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline->mPipelineLayoutInfo.setLayoutCount         = 1;
    pipeline->mPipelineLayoutInfo.pSetLayouts            = descLayouts;
    pipeline->mPipelineLayoutInfo.pushConstantRangeCount = 0;

    pipeline->build();
    return pipeline;
}
void VulkanApp::recordCommandBuffers()
{
    auto     framebuffers = mSwapchain->getSwapChainFramebuffers();
    auto     extent       = mSwapchain->getSwapChainExtent();
    uint32_t count        = mSwapchain->getImageCount();

    mCommandBuffers.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        mCommandBuffers[i] = std::make_shared<vulkancore::CommandBuffer>(mDevice, mCommandPool);

        // 开始录制命令
        mCommandBuffers[i]->beginCommandBuffer();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color       = {1.0f, 1.0f, 1.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass        = mRenderPass->getRenderPass();
        renderPassBeginInfo.framebuffer       = framebuffers[i];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = extent;
        renderPassBeginInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues      = clearValues.data();

        mCommandBuffers[i]->beginRenderPass(renderPassBeginInfo);

        // 绑定图形管线
        mCommandBuffers[i]->bindGraphicPipeline(mPipeline);

        // 绑定描述符集
        mCommandBuffers[i]->bindDescriptorSets(mPipeline, mDescriptorManager->getDescriptorSet(i));

        //VkBuffer vertexBuffers[] = {mVertexBuffer->getBuffer()};
        //VkDeviceSize offsets[]       = {0};
        //vkCmdBindVertexBuffers(mCommandBuffers[i]->getCommandBuffer(), 0, 1, vertexBuffers, offsets);

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = static_cast<float>(extent.height);
        viewport.width    = static_cast<float>(extent.width);
        viewport.height   = -static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(mCommandBuffers[i]->getCommandBuffer(), 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        vkCmdSetScissor(mCommandBuffers[i]->getCommandBuffer(), 0, 1, &scissor);

        mModel->draw(mCommandBuffers[i]);

        mCommandBuffers[i]->endRenderPass();
        mCommandBuffers[i]->endCommandBuffer();
    }
}

void VulkanApp::drawFrame()
{
    // 1. 等待GPU 将 Fence 置为 Signaled 
    mFences[mCurrentFrame]->waitForFence();

    // 2. 由Vulkan驱动决定下一张被渲染的交换链图像
    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        mDevice->getDevice(), 
        mSwapchain->getSwapChain(), 
        UINT64_MAX, 
        mImageAvailableSemaphores[mCurrentFrame]->getSemaphore(),   // 此时 imageIndex 还没有获取  同步资源只能使用mCurrentFrame获取
        VK_NULL_HANDLE, 
        &imageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
    {
        recreateSwapchain();
        mFramebufferResized = false;
        return;
    }

    // 3. 手动重置Fence
    mFences[mCurrentFrame]->resetFence();

    // 获取已经录制好的命令
    VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex]->getCommandBuffer();

    VkSemaphore          waitSemphores[]    = {mImageAvailableSemaphores[mCurrentFrame]->getSemaphore()};
    VkSemaphore          signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]->getSemaphore()};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};             // 只有在向颜色附件写入时才等待

    // 4. 提交
    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemphores;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffer;
    submitInfo.pWaitDstStageMask    = waitStages;

    vkQueueSubmit(mDevice->getGraphicQueue(), 1, &submitInfo, mFences[mCurrentFrame]->getFence());

    VkSwapchainKHR swapchains[] = {mSwapchain->getSwapChain()};

    // 5. 绘制完成(等signalSemaphores)  但是可以先提交呈现请求
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &imageIndex;

    result = vkQueuePresentKHR(mDevice->getPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mFramebufferResized)
    {
        recreateSwapchain();
        mFramebufferResized = false;
        return;
    }

    // 6. 下一帧使用的同步资源下标
    mCurrentFrame = (mCurrentFrame + 1) % mSwapchain->getImageCount();
}

float VulkanApp::getFrameTime()
{
    static double lastTime = 0.0f;
    double        currentTime = glfwGetTime();
    float         deltaTime   = static_cast<float>(currentTime - lastTime);
    lastTime                  = currentTime;
    return deltaTime;
}

void VulkanApp::updateUBO(float dt)
{
    mRotation += dt * glm::radians(45.0f);

    ObjectUBO obj;
    obj.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    obj.model = glm::rotate(obj.model, mRotation, glm::vec3(0.0f, 1.0f, 0.0f));

    CameraUBO cam;
    cam.view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    auto extent = mSwapchain->getSwapChainExtent();

    float aspect = static_cast<float>(extent.width) / extent.height;
    cam.proj     = glm::perspective(glm::radians(90.0f), aspect, 0.1f, 100.0f);

    mDescriptorManager->updateObjectUBO(obj, mCurrentFrame);
    mDescriptorManager->updateCameraUBO(cam, mCurrentFrame);
}

} // namespace VKFW
