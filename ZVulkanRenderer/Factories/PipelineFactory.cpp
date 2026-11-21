#include "PipelineFactory.h"

namespace VKFW::factories
{
std::shared_ptr<vulkancore::Pipeline> PipelineFactory::MakeSceneBase(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::RenderPass> &renderPass)
{
    auto pipeline = std::make_shared<vulkancore::Pipeline>(device, renderPass);
    fillCommon(pipeline);

    // // 剔除背面，按顺时针顶点顺序判定正面
    pipeline->mRasterState.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline->mRasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // 必须与渲染通道的采样数一致；离屏 MSAA 渲染通道
    // 使用设备支持的最大采样数
    pipeline->mMultisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline->mMultisampleState.sampleShadingEnable  = VK_FALSE;
    pipeline->mMultisampleState.rasterizationSamples = device->getMaxUsableSampleCount();

    // 开启深度测试；天空盒后续会关闭深度写入，并使用 LESS_OR_EQUAL
    pipeline->mDepthStencilState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline->mDepthStencilState.depthTestEnable  = VK_TRUE;
    pipeline->mDepthStencilState.depthWriteEnable = VK_TRUE;
    pipeline->mDepthStencilState.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

    return pipeline; // 尚未构建，由调用者完成配置并调用 build()
}

std::shared_ptr<vulkancore::Pipeline> PipelineFactory::MakeFullScreenBase(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::RenderPass> &renderPass)
{
    auto pipeline = std::make_shared<vulkancore::Pipeline>(device, renderPass);
    fillCommon(pipeline);

    // 一个生成的三角形
    pipeline->mRasterState.cullMode  = VK_CULL_MODE_BACK_BIT;
    pipeline->mRasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // 后处理通道使用单采样（MSAA 已在前面的阶段完成解析）
    pipeline->mMultisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline->mMultisampleState.sampleShadingEnable  = VK_FALSE;
    pipeline->mMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 全屏图像拷贝既不进行深度测试，也不写入深度
    pipeline->mDepthStencilState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipeline->mDepthStencilState.depthTestEnable  = VK_FALSE;
    pipeline->mDepthStencilState.depthWriteEnable = VK_FALSE;

    return pipeline; // 尚未构建，由调用者完成配置并调用 build()
}

void PipelineFactory::fillCommon(const std::shared_ptr<vulkancore::Pipeline> &pipeline)
{
    // === 输入装配 ===
	// 每3个顶点组成一个三角形 没有图元重启
    pipeline->mAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline->mAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline->mAssemblyState.primitiveRestartEnable = VK_FALSE;

    // === 动态状态与视口 ===
    // 视口和裁剪区域 在整个项目都是动态的  因此窗口大小改变时无需重建管线
    pipeline->mDynamicStates                  = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    pipeline->mDynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline->mDynamicState.dynamicStateCount = static_cast<uint32_t>(pipeline->mDynamicStates.size());
    pipeline->mDynamicState.pDynamicStates    = pipeline->mDynamicStates.data();
    // 视口和裁剪区域各一个，具体参数在命令缓冲区中动态设置
    pipeline->mViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline->mViewportState.viewportCount = 1;
    pipeline->mViewportState.scissorCount  = 1;

    // === 光栅化 ===
    // 光栅化器默认配置；各预设根据需要覆盖剔除模式
    pipeline->mRasterState.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline->mRasterState.depthClampEnable        = VK_FALSE;
    pipeline->mRasterState.rasterizerDiscardEnable = VK_FALSE;
    pipeline->mRasterState.polygonMode             = VK_POLYGON_MODE_FILL;
    pipeline->mRasterState.lineWidth               = 1.0f;
    //pipeline->mRasterState.cullMode                = VK_CULL_MODE_BACK_BIT;
    pipeline->mRasterState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipeline->mRasterState.depthBiasEnable         = VK_FALSE;

    // === 多重采样：当前关闭，后续启用 MSAA ===
    pipeline->mMultisampleState.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline->mMultisampleState.sampleShadingEnable  = VK_FALSE;
    pipeline->mMultisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // === 颜色混合：不混合，直接用输出颜色覆盖 ===
    // 一个不透明颜色附件，不启用混合
    // TODO：透明材质需要在对应管线中启用颜色混合
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable    = VK_FALSE;
    pipeline->pushBlendAttachment(blendAttachment);

    pipeline->mBlendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline->mBlendState.logicOpEnable   = VK_FALSE;
    pipeline->mBlendState.attachmentCount = static_cast<uint32_t>(pipeline->mBlendAttachmentStates.size());
    pipeline->mBlendState.pAttachments    = pipeline->mBlendAttachmentStates.data();

    // 默认不配置顶点输入；SceneBase 的调用者根据节点填充
    pipeline->mVertexInputState.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipeline->mVertexInputState.vertexBindingDescriptionCount   = 0;
    pipeline->mVertexInputState.pVertexBindingDescriptions      = nullptr;
    pipeline->mVertexInputState.vertexAttributeDescriptionCount = 0;
    pipeline->mVertexInputState.pVertexAttributeDescriptions    = nullptr;

    // 默认管线布局为空；调用者设置自己的描述符集布局
    pipeline->mPipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline->mPipelineLayoutInfo.setLayoutCount = 0;
    pipeline->mPipelineLayoutInfo.pSetLayouts    = nullptr;
}
} // namespace VKFW::factories