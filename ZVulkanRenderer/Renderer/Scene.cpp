#include "Scene.h"
#include "../VulkanCore/Sampler.h"
#include "../VulkanCore/Shader.h"

namespace VKFW::renderer
{

Scene::Scene(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::CommandPool> &commandPool, const std::shared_ptr<vulkancore::SwapChain> &swapChain, const std::shared_ptr<vulkancore::RenderPass> &swapChainRenderPass, const std::shared_ptr<OffscreenRenderTarget> &offscreenRT) : mDevice(device), mCommandPool(commandPool), mSwapChain(swapChain), mSwapChainRenderPass(swapChainRenderPass), mOffscreenRT(offscreenRT) {}

void Scene::init()
{
    initStaticResources();
    initResourcesAndObjectNodes();
    buildPipelines();
    recordCommandBuffers();
}
// 模型、纹理和相机只创建一次，窗口变化时继续使用。
void Scene::initStaticResources()
{
    if (mStaticReady)
        return;

    // 创建模型、纹理和渲染节点。
    auto model = std::make_shared<Model>();
    model->loadGltfModel("../../../Assets/DamagedHelmet/DamagedHelmet.gltf", mDevice);

    // 头盔二维贴图使用重复寻址。颜色空间处理
    // 与参考着色器保持一致：基础颜色以 UNORM 原始数值读取，
    // 随后在片元着色器中进行 Gamma 解码。因此
    // 这里所有贴图都使用 UNORM 格式；着色器中的 GammaDecode 处理
    // 基础颜色和自发光，法线、金属度粗糙度、AO 等线性数据直接读取。
    // 如果基础颜色使用 SRGB 格式，硬件会自动线性化，再加上着色器解码，
    // 就会重复转换，使头盔显得过暗。
    auto repeatSampler = std::make_shared<vulkancore::Sampler>(mDevice, /*isCubeMap*/ false, /*isRepeat*/ true);

    auto albedoImg   = vulkancore::Image::createImageFromFile(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_albedo.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    auto normalImg   = vulkancore::Image::createImageFromFile(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_normal.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    auto mrImg       = vulkancore::Image::createImageFromFile(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_metalRoughness.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    auto aoImg       = vulkancore::Image::createImageFromFile(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_AO.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    auto emissiveImg = vulkancore::Image::createImageFromFile(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_emissive.jpg", VK_FORMAT_R8G8B8A8_UNORM);
    mPBRAlbedo       = vulkancore::Texture::createFromImage(mDevice, albedoImg, repeatSampler);
    mPBRNormal       = vulkancore::Texture::createFromImage(mDevice, normalImg, repeatSampler);
    mPBRMetalRough   = vulkancore::Texture::createFromImage(mDevice, mrImg, repeatSampler);
    mPBRAO           = vulkancore::Texture::createFromImage(mDevice, aoImg, repeatSampler);
    mPBREmissive     = vulkancore::Texture::createFromImage(mDevice, emissiveImg, repeatSampler);

    //mObjectTexture = vulkancore::Texture::create(mDevice, mCommandPool, "../../../Assets/DamagedHelmet/Default_albedo.jpg");
    
    mPBRNode = std::make_shared<RenderObjectNode>();
    mPBRNode->mModels.push_back(model);
    mPBRNode->setShaderFiles("spvs/pbr_vert.spv", "spvs/pbr_frag.spv");

    // 相机位于 (0, 1.5, 5)，看向原点。
    mMainCamera = std::make_shared<Camera>();
    mMainCamera->init(glm::vec3(0.0f, 0.0f, 0.0f), glm::length(glm::vec3(0.0f, 1.5f, 5.0f)), glm::normalize(glm::vec3(0.0f, -1.5f, -5.0f)));
    mMainCamera->setSpeed(2.5f * 0.016f); // 60 帧时每秒移动约 2.5 个单位

    // 方向光，每帧上传到共享 set 0、binding 1。
    mLight = std::make_shared<Light>();
    mLight->setDirection(glm::vec3(-1.0f, -1.0f, -1.0f));
    mLight->setColor(glm::vec3(1.0f, 1.0f, 1.0f));
    mLight->setIntensity(2.0f);

    mIBLBaker = std::make_shared<IBLBaker>(mDevice, mCommandPool);

    mEnvCubemap = mIBLBaker->equirectToCubemap("../../../Assets/1.hdr", 512);
    mIrradianceCubemap = mIBLBaker->bakeIrradiance(mEnvCubemap, 32);
    mPrefilterCubemap  = mIBLBaker->bakePrefilter(mEnvCubemap, 128);
    mBRDFLUT           = mIBLBaker->bakeBRDFLUT(512);

    mObjectTexture = vulkancore::Texture::createFromImage(mDevice, mBRDFLUT);

    auto skyboxModel = std::make_shared<Model>();
    skyboxModel->loadModel("../../../Assets/skybox_cube.obj", mDevice);

    mSkyboxObjectNode = std::make_shared<RenderObjectNode>();
    mSkyboxObjectNode->mModels.push_back(skyboxModel);
    mSkyboxObjectNode->setShaderFiles("spvs/skybox_vert.spv", "spvs/skybox_frag.spv");

    mStaticReady = true;
}
// 创建场景中的描述符。
//   set 0：所有节点共享的相机 UBO
//   set 1：每个节点的物体 UBO 和纹理
//   全屏绘制：读取离屏渲染结果
void Scene::initResourcesAndObjectNodes()
{
    const uint32_t imageCount = mSwapChain->getImageCount();

    // set 0：所有节点共享，目前只有相机数据。
    // 后续加入灯光和 IBL 贴图。
    mGlobalDescriptorManager = std::make_shared<DescriptorManager>();
    mGlobalDescriptorManager->init(mDevice, imageCount);
    mGlobalDescriptorManager->attachCameraUBO();
    mGlobalDescriptorManager->attachLightUBO();
    mGlobalDescriptorManager->build();

    //// set 1：当前节点的模型矩阵和纹理。
    //auto objectNodeDescriptorManager = std::make_shared<DescriptorManager>();
    //objectNodeDescriptorManager->init(mDevice, imageCount);
    //objectNodeDescriptorManager->attachObjectUBO();
    //objectNodeDescriptorManager->attachTexture(mObjectTexture);
    //objectNodeDescriptorManager->build();
    //mObjectNode->mDescriptorManager = objectNodeDescriptorManager;

    // 天空盒
    auto skyboxDescriptorManager = std::make_shared<DescriptorManager>();
    skyboxDescriptorManager->init(mDevice, imageCount);
    skyboxDescriptorManager->attachObjectUBO();

    auto skyboxTexture = vulkancore::Texture::createFromImage(mDevice, mEnvCubemap, std::make_shared<vulkancore::Sampler>(mDevice));

    skyboxDescriptorManager->attachTexture(skyboxTexture);
    skyboxDescriptorManager->build();

    mSkyboxObjectNode->mDescriptorManager = skyboxDescriptorManager;


    // ---- PBR 节点 set 1：物体 UBO、三张 IBL 图像、五张头盔贴图。
    // binding 顺序必须与 pbr.slang 保持一致：
    // binding 0：物体 UBO。
    // binding 1：辐照度图；2：预过滤图；3：BRDF LUT。
    // binding 4：基础颜色；5：法线；6：金属度粗糙度；7：AO；8：自发光。
    // 三张 IBL 图像均封装为 Texture：两张 CubeMap 使用立方体视图，
    // BRDF LUT 使用二维视图。----
    auto cubeSampler   = std::make_shared<vulkancore::Sampler>(mDevice, /*isCubeMap*/ true);
    auto lutSampler    = std::make_shared<vulkancore::Sampler>(mDevice);
    auto irradianceTex = vulkancore::Texture::createFromImage(mDevice, mIrradianceCubemap, cubeSampler);
    auto prefilterTex  = vulkancore::Texture::createFromImage(mDevice, mPrefilterCubemap, cubeSampler);
    auto brdfTex       = vulkancore::Texture::createFromImage(mDevice, mBRDFLUT, lutSampler);

    auto pbrDM = std::make_shared<DescriptorManager>();
    pbrDM->init(mDevice, imageCount);
    pbrDM->attachObjectUBO();             // binding 0。
    pbrDM->attachTexture(irradianceTex);  // binding 1。
    pbrDM->attachTexture(prefilterTex);   // binding 2。
    pbrDM->attachTexture(brdfTex);        // binding 3。
    pbrDM->attachTexture(mPBRAlbedo);     // binding 4。
    pbrDM->attachTexture(mPBRNormal);     // binding 5。
    pbrDM->attachTexture(mPBRMetalRough); // binding 6。
    pbrDM->attachTexture(mPBRAO);         // binding 7。
    pbrDM->attachTexture(mPBREmissive);   // binding 8。
    pbrDM->build();
    mPBRNode->mDescriptorManager = pbrDM;
    

    // 全屏绘制读取离屏结果，窗口变化后重建描述符。
    createQuadDescriptors();
}

void Scene::createQuadDescriptors()
{
    const uint32_t count = mSwapChain->getImageCount();

    // 共用一个采样器，每张离屏图像分别包装为纹理。
    auto                                              sampler = std::make_shared<vulkancore::Sampler>(mDevice);
    std::vector<std::shared_ptr<vulkancore::Texture>> renderTargetTextures(count);
    for (uint32_t i = 0; i < count; i++)
    {
        renderTargetTextures[i] = vulkancore::Texture::createFromImage(mDevice, mOffscreenRT->getRenderTargetImage(i), sampler);
    }

    mQuadDescriptorManager = std::make_shared<DescriptorManager>();
    mQuadDescriptorManager->init(mDevice, count);
    mQuadDescriptorManager->attachPerFrameTextures(renderTargetTextures);
    mQuadDescriptorManager->build();
}
// 创建两条管线，均使用动态视口和裁剪区域。
// 窗口尺寸变化时无需重建这两条管线。
void Scene::buildPipelines()
{
    // 场景管线：将立方体画入离屏目标，开启深度测试、背面剔除和 MSAA。
    {
        mScenePipeline = factories::PipelineFactory::MakeSceneBase(mDevice, mOffscreenRT->getRenderPass());
        // PBR 头盔：剔除背面，以逆时针绕序判定正面。
        mScenePipeline->mRasterState.cullMode  = VK_CULL_MODE_BACK_BIT;
        mScenePipeline->mRasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        auto vertexShader   = std::make_shared<vulkancore::Shader>(mDevice, mPBRNode->getVertexShaderFile(), VK_SHADER_STAGE_VERTEX_BIT, "main");
        auto fragmentShader = std::make_shared<vulkancore::Shader>(mDevice, mPBRNode->getFragmentShaderFile(), VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        mScenePipeline->setShaderGroup({vertexShader, fragmentShader});

        // 从节点的模型获取顶点输入描述，在 build() 前保持数组有效。
        auto bindings                                                     = mPBRNode->mModels[0]->getPBRBindingDescriptions();
        auto attributes                                                   = mPBRNode->mModels[0]->getPBRAttributeDescriptions();
        mScenePipeline->mVertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
        mScenePipeline->mVertexInputState.pVertexBindingDescriptions      = bindings.data();
        mScenePipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        mScenePipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

        // set 0 使用场景共享数据，set 1 使用当前节点的数据。
        VkDescriptorSetLayout layouts[]                    = {mGlobalDescriptorManager->getLayout()->getLayout(), mPBRNode->mDescriptorManager->getLayout()->getLayout()};
        mScenePipeline->mPipelineLayoutInfo.setLayoutCount = 2;
        mScenePipeline->mPipelineLayoutInfo.pSetLayouts    = layouts;

        mScenePipeline->build();
    }

    // 天空盒也画到离屏目标，但不写入深度
    {
        mSkyboxPipeline = factories::PipelineFactory::MakeSceneBase(mDevice, mOffscreenRT->getRenderPass());

        mSkyboxPipeline->mDepthStencilState.depthWriteEnable = VK_FALSE;

        auto vertexShader = std::make_shared<vulkancore::Shader>(mDevice, mSkyboxObjectNode->getVertexShaderFile(), VK_SHADER_STAGE_VERTEX_BIT, "main");

        auto fragmentShader = std::make_shared<vulkancore::Shader>(mDevice, mSkyboxObjectNode->getFragmentShaderFile(), VK_SHADER_STAGE_FRAGMENT_BIT, "main");

        mSkyboxPipeline->setShaderGroup({vertexShader, fragmentShader});

        // 下一步在这里补顶点输入、描述符布局，再 build()
        auto bindings   = mSkyboxObjectNode->getVertexInputBindingDescriptions();
        auto attributes = mSkyboxObjectNode->getVertexInputAttributeDescriptions();

        mSkyboxPipeline->mVertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        mSkyboxPipeline->mVertexInputState.pVertexBindingDescriptions    = bindings.data();

        mSkyboxPipeline->mVertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        mSkyboxPipeline->mVertexInputState.pVertexAttributeDescriptions    = attributes.data();

        VkDescriptorSetLayout layouts[] = {
            mGlobalDescriptorManager->getLayout()->getLayout(),             // set 0：相机
            mSkyboxObjectNode->mDescriptorManager->getLayout()->getLayout() // set 1：天空盒
        };

        mSkyboxPipeline->mPipelineLayoutInfo.setLayoutCount = 2;
        mSkyboxPipeline->mPipelineLayoutInfo.pSetLayouts    = layouts;

        mSkyboxPipeline->build();
    }


    // 全屏管线：将离屏结果画到交换链图像。
    // 不使用顶点缓冲区和深度测试，使用单采样。
    {
        mQuadPipeline = factories::PipelineFactory::MakeFullScreenBase(mDevice, mSwapChainRenderPass);

        auto vertexShader   = std::make_shared<vulkancore::Shader>(mDevice, "spvs/screen_quad_vert.spv", VK_SHADER_STAGE_VERTEX_BIT, "main");
        auto fragmentShader = std::make_shared<vulkancore::Shader>(mDevice, "spvs/screen_quad_frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, "main");
        mQuadPipeline->setShaderGroup({vertexShader, fragmentShader});

        VkDescriptorSetLayout layouts[]                   = {mQuadDescriptorManager->getLayout()->getLayout()};
        mQuadPipeline->mPipelineLayoutInfo.setLayoutCount = 1;
        mQuadPipeline->mPipelineLayoutInfo.pSetLayouts    = layouts;

        mQuadPipeline->build();
    }
}
// 为每张交换链图像录制一份命令缓冲区。
void Scene::recordCommandBuffers()
{
    auto     framebuffers = mSwapChain->getSwapChainFramebuffers();
    auto     extent       = mSwapChain->getSwapChainExtent();
    uint32_t count        = mSwapChain->getImageCount();

    mCommandBuffers.resize(count);
    for (uint32_t i = 0; i < count; i++)
    {
        mCommandBuffers[i]            = std::make_shared<vulkancore::CommandBuffer>(mDevice, mCommandPool);
        VkCommandBuffer commandBuffer = mCommandBuffers[i]->getCommandBuffer();

        mCommandBuffers[i]->beginCommandBuffer();

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = static_cast<float>(extent.height);
        viewport.width    = static_cast<float>(extent.width);
        viewport.height   = -static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        // 第一遍：将场景画入离屏目标，并完成 MSAA 解析。
        std::array<VkClearValue, 3> offscreenClear{};
        offscreenClear[0].color        = {0.0f, 0.2f, 0.4f, 1.0f};
        offscreenClear[1].color        = {0.0f, 0.2f, 0.4f, 1.0f};
        offscreenClear[2].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo offscreenInfo{};
        offscreenInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        offscreenInfo.renderPass        = mOffscreenRT->getRenderPass()->getRenderPass();
        offscreenInfo.framebuffer       = mOffscreenRT->getOffScreenFramebuffers()[i];
        offscreenInfo.renderArea.offset = {0, 0};
        offscreenInfo.renderArea.extent = {mOffscreenRT->getWidth(), mOffscreenRT->getHeight()};
        offscreenInfo.clearValueCount   = static_cast<uint32_t>(offscreenClear.size());
        offscreenInfo.pClearValues      = offscreenClear.data();

        mCommandBuffers[i]->beginRenderPass(offscreenInfo);
        mCommandBuffers[i]->bindGraphicPipeline(mScenePipeline);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // 绑定 set 0：所有节点共享的相机数据。
        mCommandBuffers[i]->bindDescriptorSets(mScenePipeline, mGlobalDescriptorManager->getDescriptorSet(i), 0);


        // 先画天空盒
        mCommandBuffers[i]->bindGraphicPipeline(mSkyboxPipeline);
        mCommandBuffers[i]->bindDescriptorSets(mSkyboxPipeline, mSkyboxObjectNode->mDescriptorManager->getDescriptorSet(i), 1);
        mSkyboxObjectNode->draw(mCommandBuffers[i]);

        // 切回场景管线，接着画兔子
        mCommandBuffers[i]->bindGraphicPipeline(mScenePipeline);


        // 绑定当前节点的 set 1，然后绘制；多个节点依次执行。
        mCommandBuffers[i]->bindDescriptorSets(mScenePipeline, mPBRNode->mDescriptorManager->getDescriptorSet(i), 1);
        mPBRNode->draw(mCommandBuffers[i]);

        mCommandBuffers[i]->endRenderPass();
        // RenderPass 自动转换布局，解析结果现在可供着色器读取。

        // 第二遍：通过全屏三角形将离屏结果画到交换链图像。
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {0.0f, 0.0f, 0.0f, 1.0f};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = mSwapChainRenderPass->getRenderPass();
        renderPassInfo.framebuffer       = framebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues      = clearValues.data();

        mCommandBuffers[i]->beginRenderPass(renderPassInfo);
        mCommandBuffers[i]->bindGraphicPipeline(mQuadPipeline);
        mCommandBuffers[i]->bindDescriptorSets(mQuadPipeline, mQuadDescriptorManager->getDescriptorSet(i), 0);

        VkViewport quadViewport{};
        quadViewport.x        = 0.0f;
        quadViewport.y        = 0.0f;
        quadViewport.width    = static_cast<float>(extent.width);
        quadViewport.height   = static_cast<float>(extent.height);
        quadViewport.minDepth = 0.0f;
        quadViewport.maxDepth = 1.0f;

        vkCmdSetViewport(commandBuffer, 0, 1, &quadViewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        mCommandBuffers[i]->endRenderPass();

        mCommandBuffers[i]->endCommandBuffer();
    }
}
// 每帧更新。
void Scene::update(float dt, uint32_t frameIndex)
{
    // 更新旋转动画。
    mRotation += dt * glm::radians(45.0f);
    mPBRNode->setRotationEuler(0.0f, mRotation, 0.0f);

    // 更新 set 0 中的相机数据。
    auto  extent = mSwapChain->getSwapChainExtent();
    float aspect = static_cast<float>(extent.width) / extent.height;
    mMainCamera->setPerspective(45.0f, aspect, 0.1f, 100.0f);

    CameraUBO cameraData{};
    cameraData.view = mMainCamera->getViewMatrix();
    cameraData.proj = mMainCamera->getProjectMatrix();
    cameraData.cameraPos = mMainCamera->getCameraPosition();
    mGlobalDescriptorManager->updateCameraUBO(cameraData, frameIndex);

    LightUBO light = mLight->getUBO();
    mGlobalDescriptorManager->updateLightUBO(light, frameIndex);


    // 更新 set 1 中的模型矩阵和法线矩阵。
    mPBRNode->updateTransformIfNeeded();
    ObjectUBO objectData{};
    objectData.model        = mPBRNode->getModelMatrix();
    objectData.normalMatrix = mPBRNode->getNormalMatrix();
    mPBRNode->mDescriptorManager->updateObjectUBO(objectData, frameIndex);

    glm::vec3 cameraPosition = glm::vec3(mMainCamera->getCameraPosition());

    ObjectUBO skyboxData{};
    skyboxData.model        = glm::translate(glm::mat4(1.0f), cameraPosition);
    skyboxData.normalMatrix = glm::mat4(1.0f);

    mSkyboxObjectNode->mDescriptorManager->updateObjectUBO(skyboxData, frameIndex);
}
// 窗口变化后，重建指向离屏图像的描述符，
// 并使用新的尺寸和 Framebuffer 重新录制命令。
void Scene::onResize()
{
    createQuadDescriptors();

    mCommandBuffers.clear();
    recordCommandBuffers();
}

} // namespace VKFW::renderer
