#include "IBLBaker.h"

namespace VKFW::renderer
{

IBLBaker::IBLBaker(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool) : mDevice(device), mCommandPool(commandPool)
{
    initCaptureCameras();
}

std::shared_ptr<vulkancore::Image> IBLBaker::equirectToCubemap(const std::string &hdrPath, uint32_t faceSize, const std::string &vertexShaderFile, const std::string &fragmentShaderFile)
{
    auto cubemap = vulkancore::Image::createCubeMapImage(mDevice, faceSize, faceSize, VK_FORMAT_R32G32B32A32_SFLOAT);

    cubemap->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mCommandPool);

    auto equirectImage = vulkancore::Image::createHDRImageFromFile(mDevice, mCommandPool, hdrPath);

    auto equirectTexture = vulkancore::Texture::createFromImage(mDevice, equirectImage, std::make_shared<vulkancore::Sampler>(mDevice));

    auto skybox = std::make_shared<Model>();
    skybox->loadModel("../../../Assets/skybox_cube.obj", mDevice);

    auto renderTarget = std::make_shared<OffscreenRenderTarget>(mDevice, mCommandPool, faceSize, faceSize, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    
    // set 0：当前面的相机矩阵
    auto globalDescriptorManager = std::make_shared<DescriptorManager>();
    globalDescriptorManager->init(mDevice, 1);
    globalDescriptorManager->attachCameraUBO();
    globalDescriptorManager->build();

    // set 1：模型矩阵和输入的 HDR 全景纹理
    auto objectDescriptorManager = std::make_shared<DescriptorManager>();
    objectDescriptorManager->init(mDevice, 1);
    objectDescriptorManager->attachObjectUBO();
    objectDescriptorManager->attachTexture(equirectTexture);
    objectDescriptorManager->build();

    auto pipeline = factories::PipelineFactory::MakeSceneBase(mDevice, renderTarget->getRenderPass());

    auto vertexShader = std::make_shared<vulkancore::Shader>(mDevice, vertexShaderFile, VK_SHADER_STAGE_VERTEX_BIT, "main");

    auto fragmentShader = std::make_shared<vulkancore::Shader>(mDevice, fragmentShaderFile, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

    pipeline->setShaderGroup({vertexShader, fragmentShader});

    auto bindings   = skybox->getBindingDescriptions();
    auto attributes = skybox->getAttributeDescriptions();

    pipeline->mVertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    pipeline->mVertexInputState.pVertexBindingDescriptions    = bindings.data();

    pipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    pipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

    VkDescriptorSetLayout layouts[] = {
        globalDescriptorManager->getLayout()->getLayout(), // set 0
        objectDescriptorManager->getLayout()->getLayout()  // set 1
    };

    pipeline->mPipelineLayoutInfo.setLayoutCount = 2;
    pipeline->mPipelineLayoutInfo.pSetLayouts    = layouts;

    pipeline->build();

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = static_cast<float>(faceSize);
    viewport.width    = static_cast<float>(faceSize);
    viewport.height   = -static_cast<float>(faceSize);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {faceSize, faceSize};

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color        = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[1].color        = {0.0f, 0.0f, 0.0f, 0.0f};
    clearValues[2].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderTarget->getRenderPass()->getRenderPass();
    renderPassInfo.framebuffer       = renderTarget->getOffScreenFramebuffers()[0];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {faceSize, faceSize};
    renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues      = clearValues.data();

    ObjectUBO objectData{};
    objectData.model        = glm::mat4(1.0f);
    objectData.normalMatrix = glm::mat4(1.0f);

    objectDescriptorManager->updateObjectUBO(objectData, 0);

    for (uint32_t face = 0; face < 6; face++)
    {
        CameraUBO cameraData{};
        cameraData.view      = mCaptureCameras[face].getViewMatrix();
        cameraData.proj      = mCaptureCameras[face].getProjectMatrix();
        cameraData.cameraPos = mCaptureCameras[face].getCameraPosition();

        globalDescriptorManager->updateCameraUBO(cameraData, 0);

        // 接下来在这里录制当前面的绘制和复制命令
        auto commandBuffer = std::make_shared<vulkancore::CommandBuffer>(mDevice, mCommandPool);

        commandBuffer->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        commandBuffer->beginRenderPass(renderPassInfo);

        commandBuffer->bindGraphicPipeline(pipeline);

        VkCommandBuffer rawCommandBuffer = commandBuffer->getCommandBuffer();
        vkCmdSetViewport(rawCommandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(rawCommandBuffer, 0, 1, &scissor);

        commandBuffer->bindDescriptorSets(pipeline, globalDescriptorManager->getDescriptorSet(0), 0);

        commandBuffer->bindDescriptorSets(pipeline, objectDescriptorManager->getDescriptorSet(0), 1);

        skybox->draw(commandBuffer);

        commandBuffer->endRenderPass();

        commandBuffer->copyImageToCubeFace(renderTarget->getRenderTargetImage(0)->getImage(), cubemap->getImage(), faceSize, faceSize, face, 0);

        commandBuffer->endCommandBuffer();
        commandBuffer->submitCommandBuffer(mDevice->getGraphicQueue());
        commandBuffer->waitCommandBuffer(mDevice->getGraphicQueue());
    }
    
    cubemap->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, mCommandPool);

    return cubemap;
}

std::shared_ptr<vulkancore::Image> IBLBaker::bakeIrradiance(const std::shared_ptr<vulkancore::Image> &envCubemap, uint32_t faceSize, const std::string &vertSpv, const std::string &fragSpv)
{
    auto diffuseIrradiance = vulkancore::Image::createCubeMapImage(mDevice, faceSize, faceSize, VK_FORMAT_R32G32B32A32_SFLOAT);
    diffuseIrradiance->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mCommandPool);

    auto envTex = vulkancore::Texture::createFromImage(mDevice, envCubemap, std::make_shared<vulkancore::Sampler>(mDevice));

    auto skybox = std::make_shared<Model>();
    skybox->loadModel("../../../Assets/skybox_cube.obj", mDevice);

    auto rt = std::make_shared<OffscreenRenderTarget>(mDevice, mCommandPool, faceSize, faceSize, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    auto set0 = std::make_shared<DescriptorManager>();
    set0->init(mDevice, 1);
    set0->attachCameraUBO();
    set0->build();

    auto set1 = std::make_shared<DescriptorManager>();
    set1->init(mDevice, 1);
    set1->attachObjectUBO();
    set1->attachTexture(envTex);
    set1->build();

    // 封装好的离屏RT 已准备好renderpass和framebuffer 默认MSAA管线 
    auto pipeline = factories::PipelineFactory::MakeSceneBase(mDevice, rt->getRenderPass());
    pipeline->mRasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    auto vert = std::make_shared<vulkancore::Shader>(mDevice, vertSpv, VK_SHADER_STAGE_VERTEX_BIT, "main");
    auto frag = std::make_shared<vulkancore::Shader>(mDevice, fragSpv, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

    pipeline->setShaderGroup({vert, frag});

    auto bindings = skybox->getBindingDescriptions();
    auto attributes = skybox->getAttributeDescriptions();
    pipeline->mVertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    pipeline->mVertexInputState.pVertexBindingDescriptions      = bindings.data();
    pipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    pipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

    VkDescriptorSetLayout layouts[]              = {set0->getLayout()->getLayout(), set1->getLayout()->getLayout()};
    pipeline->mPipelineLayoutInfo.setLayoutCount = 2;
    pipeline->mPipelineLayoutInfo.pSetLayouts    = layouts;
    pipeline->build();

    // 辐照度烘焙使用不翻转 Y 方向的视口，与原实现保持一致。
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(faceSize);
    viewport.height   = static_cast<float>(faceSize);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {faceSize, faceSize};

    std::array<VkClearValue, 3> clears{};
    clears[0].color        = {0, 0, 0, 0};
    clears[1].color        = {0, 0, 0, 0};
    clears[2].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass        = rt->getRenderPass()->getRenderPass();
    rpInfo.framebuffer       = rt->getOffScreenFramebuffers()[0];
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {faceSize, faceSize};
    rpInfo.clearValueCount   = static_cast<uint32_t>(clears.size());
    rpInfo.pClearValues      = clears.data();

    ObjectUBO obj{};
    obj.model        = glm::mat4(1.0f);
    obj.normalMatrix = glm::mat4(1.0f);
    set1->updateObjectUBO(obj, 0);

    // 辐照度和预过滤烘焙对第 2、3 面使用与 HDR 转 CubeMap 不同的相机配置。
    // 在本次烘焙的相机副本中调整这两个面的观察方向和参考向上方向，
    // 不修改原来的六个拍摄相机。
    Camera caps[6];
    for (int i = 0; i < 6; i++)
        caps[i] = mCaptureCameras[i];
    caps[2].lookAt({0, 0, 0}, {0, 1, 0}, {0, 0, 1});
    caps[3].lookAt({0, 0, 0}, {0, -1, 0}, {0, 0, -1});
    for (int i = 0; i < 6; i++)
        caps[i].setPerspective(90.0f, 1.0f, 0.1f, 100.0f);

    for (uint32_t face = 0; face < 6; face++)
    {
        CameraUBO cam{};
        cam.view = caps[face].getViewMatrix();
        cam.proj = caps[face].getProjectMatrix();
        cam.cameraPos = caps[face].getCameraPosition();
        set0->updateCameraUBO(cam, 0);
        
        auto cmd = std::make_shared<vulkancore::CommandBuffer>(mDevice, mCommandPool);
        cmd->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        cmd->beginRenderPass(rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        cmd->bindGraphicPipeline(pipeline);

        VkCommandBuffer raw = cmd->getCommandBuffer();
        vkCmdSetViewport(raw, 0, 1, &viewport);
        vkCmdSetScissor(raw, 0, 1, &scissor);

        cmd->bindDescriptorSets(pipeline, set0->getDescriptorSet(0), 0);
        cmd->bindDescriptorSets(pipeline, set1->getDescriptorSet(0), 1);

        skybox->draw(cmd);
        cmd->endRenderPass();

        cmd->copyImageToCubeFace(rt->getRenderTargetImage(0)->getImage(), diffuseIrradiance->getImage(), faceSize, faceSize, face, 0);
        
        cmd->endCommandBuffer();
        cmd->submitCommandBuffer(mDevice->getGraphicQueue());
        cmd->waitCommandBuffer(mDevice->getGraphicQueue());
    }

    diffuseIrradiance->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, mCommandPool);

    return diffuseIrradiance;
}

std::shared_ptr<vulkancore::Image> IBLBaker::bakePrefilter(const std::shared_ptr<vulkancore::Image> envCubemap, uint32_t baseSize, uint32_t mipLevels, const std::string &vertSpv, const std::string &fragSpv)
{
    // 创建图像
    auto prefilter = vulkancore::Image::createCubeMapImage(mDevice, baseSize, baseSize, VK_FORMAT_R32G32B32A32_SFLOAT, mipLevels);
    prefilter->transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, mCommandPool);

    // 使用着色器烘焙 录制cmd 循环copy到图像
    auto envTex = vulkancore::Texture::createFromImage(mDevice, envCubemap, std::make_shared<vulkancore::Sampler>(mDevice));
    auto skybox = std::make_shared<Model>();
    skybox->loadModel("../../../Assets/skybox_cube.obj", mDevice);

    auto set0 = std::make_shared<DescriptorManager>();
    set0->init(mDevice, 1);
    set0->attachCameraUBO();
    set0->build();

    auto set1 = std::make_shared<DescriptorManager>();
    set1->init(mDevice, 1);
    set1->attachObjectUBO();
    set1->attachTexture(envTex);
    set1->build();

    ObjectUBO obj{};
    obj.model        = glm::mat4(1.0f);
    obj.normalMatrix = glm::mat4(1.0f);
    set1->updateObjectUBO(obj, 0);

    // 推送常量
    struct PushConstants
    {
        float roughness;
    };
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset     = 0;
    pushRange.size       = sizeof(PushConstants);

    Camera caps[6];
    for (int i = 0; i < 6; i++)
        caps[i] = mCaptureCameras[i];
    caps[2].lookAt({0, 0, 0}, {0, 1, 0}, {0, 0, 1});
    caps[3].lookAt({0, 0, 0}, {0, -1, 0}, {0, 0, -1});
    for (int i = 0; i < 6; i++)
        caps[i].setPerspective(90.0f, 1.0f, 0.1f, 100.0f);

    for (uint32_t mip = 0; mip < mipLevels; mip++)
    {
        uint32_t mipSize    = static_cast<uint32_t>(baseSize * pow(0.5f, mip));
        float    roughness = (mipLevels > 1) ? static_cast<float>(mip) / static_cast<float>(mipLevels - 1) : 0.0f;

        auto rt = std::make_shared<OffscreenRenderTarget>(mDevice, mCommandPool, mipSize, mipSize, 1, VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        auto pipeline = factories::PipelineFactory::MakeSceneBase(mDevice, rt->getRenderPass());
        
        pipeline->mRasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        auto vert = std::make_shared<vulkancore::Shader>(mDevice, vertSpv, VK_SHADER_STAGE_VERTEX_BIT, "main");
        auto frag = std::make_shared<vulkancore::Shader>(mDevice, fragSpv, VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        pipeline->setShaderGroup({vert, frag});

        auto bindings                                               = skybox->getBindingDescriptions();
        auto attributes                                             = skybox->getAttributeDescriptions();
        pipeline->mVertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
        pipeline->mVertexInputState.pVertexBindingDescriptions      = bindings.data();
        pipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        pipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

        VkDescriptorSetLayout layouts[]                      = {set0->getLayout()->getLayout(), set1->getLayout()->getLayout()};
        pipeline->mPipelineLayoutInfo.setLayoutCount         = 2;
        pipeline->mPipelineLayoutInfo.pSetLayouts            = layouts;
        pipeline->mPipelineLayoutInfo.pushConstantRangeCount = 1;
        pipeline->mPipelineLayoutInfo.pPushConstantRanges    = &pushRange;
        pipeline->build();

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(mipSize);
        viewport.height   = static_cast<float>(mipSize);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {mipSize, mipSize};

        std::array<VkClearValue, 3> clears{};
        clears[0].color        = {0, 0, 0, 0};
        clears[1].color        = {0, 0, 0, 0};
        clears[2].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass        = rt->getRenderPass()->getRenderPass();
        rpInfo.framebuffer       = rt->getOffScreenFramebuffers()[0];
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = {mipSize, mipSize};
        rpInfo.clearValueCount   = static_cast<uint32_t>(clears.size());
        rpInfo.pClearValues      = clears.data();

        PushConstants pcData{};
        pcData.roughness = roughness;

        for (uint32_t face = 0; face < 6; face++)
        {
            CameraUBO cam{};
            cam.view      = caps[face].getViewMatrix();
            cam.proj      = caps[face].getProjectMatrix();
            cam.cameraPos = caps[face].getCameraPosition();
            set0->updateCameraUBO(cam, 0);

            auto cmd = std::make_shared<vulkancore::CommandBuffer>(mDevice, mCommandPool);
            cmd->beginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

            cmd->beginRenderPass(rpInfo, VK_SUBPASS_CONTENTS_INLINE);
            cmd->bindGraphicPipeline(pipeline);

                VkCommandBuffer raw = cmd->getCommandBuffer();
            vkCmdSetViewport(raw, 0, 1, &viewport);
            vkCmdSetScissor(raw, 0, 1, &scissor);

            cmd->bindDescriptorSets(pipeline, set0->getDescriptorSet(0), 0);
            cmd->bindDescriptorSets(pipeline, set1->getDescriptorSet(0), 1);

            cmd->pushConstants(pipeline->getPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pcData);

            skybox->draw(cmd);
            cmd->endRenderPass();

            cmd->copyImageToCubeFace(rt->getRenderTargetImage(0)->getImage(), prefilter->getImage(), mipSize, mipSize, face, mip);

            cmd->endCommandBuffer();
            cmd->submitCommandBuffer(mDevice->getGraphicQueue());
            cmd->waitCommandBuffer(mDevice->getGraphicQueue());
        }
    }
    prefilter->transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, mCommandPool);

    return prefilter;
}

void IBLBaker::initCaptureCameras()
{
    mCaptureCameras[0].lookAt({0, 0, 0}, {1, 0, 0}, {0, -1, 0});
    mCaptureCameras[1].lookAt({0, 0, 0}, {-1, 0, 0}, {0, -1, 0});
    mCaptureCameras[2].lookAt({0, 0, 0}, {0, -1, 0}, {0, 0, -1});
    mCaptureCameras[3].lookAt({0, 0, 0}, {0, 1, 0}, {0, 0, 1});
    mCaptureCameras[4].lookAt({0, 0, 0}, {0, 0, 1}, {0, -1, 0});
    mCaptureCameras[5].lookAt({0, 0, 0}, {0, 0, -1}, {0, -1, 0});

    for (int i = 0; i < 6; i++)
        mCaptureCameras[i].setPerspective(90.0f, 1.0f, 0.1f, 100.0f);
}

} // namespace VKFW::renderer