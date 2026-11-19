#pragma once
#include "../Common.h"
#include "../VulkanCore/Device.h"
#include "../VulkanCore/Pipeline.h"
#include "../VulkanCore/RenderPass.h"

// 轻量级管线预设工厂。
//
// 每个 Make*Base 函数都会返回一个已经填充好通用配置、
// 但尚未完成构建（build）的管线对象。
//
// 调用者需要自行设置该管线特有的配置：
//   - 着色器（Shaders）
//   - 顶点输入（Vertex Input）
//   - 描述符布局（Descriptor Layouts）
//   - 其他需要覆盖的管线状态
//
// 完成配置后，由调用者自行调用 build() 创建 Vulkan 管线。
//
// 两种预设统一了整个项目的视口约定：
//
// SceneBase：
//   用于场景几何渲染阶段，采用翻转视口（FLIPPED）。
//   GLM 生成的矩阵可以直接使用，无需额外修改。
//
// FullscreenBase：
//   用于全屏渲染和后处理阶段，采用默认视口（DEFAULT）。
//   可以直接使用标准全屏三角形，无需修改其坐标。
namespace VKFW::factories
{
class PipelineFactory
{
  public:
    // 场景几何渲染管线预设：
    //
    //   - 开启深度测试和深度写入，比较方式为 LESS
    //   - 开启背面剔除（Back-face Culling）
    //   - 使用逆时针方向（CCW）定义正面
    //   - MSAA 采样数设置为设备支持的最大值
    //     （与离屏 MSAA 渲染通道保持一致）
    //   - 视口和裁剪区域采用动态状态
    //   - 配置一个不启用颜色混合的颜色附件
    //
    // 调用者仍需设置：
    //   - 着色器
    //   - 顶点输入
    //   - 管线布局
    static std::shared_ptr<vulkancore::Pipeline> MakeSceneBase(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::RenderPass> &renderPass);

    // 全屏渲染 / 后处理管线预设：
    //
    //   - 不需要顶点输入
    //     （通过 SV_VertexID 生成全屏三角形）
    //   - 关闭深度测试
    //   - 关闭面剔除（Cull NONE）
    //   - 使用单采样（不启用 MSAA）
    //   - 视口和裁剪区域采用动态状态
    //   - 配置一个不启用颜色混合的颜色附件
    //
    // 调用者仍需设置：
    //   - 着色器
    //   - 管线布局
    static std::shared_ptr<vulkancore::Pipeline> MakeFullScreenBase(const std::shared_ptr<vulkancore::Device> &device, const std::shared_ptr<vulkancore::RenderPass> &renderPass);

  private:
    // 填充所有预设管线共有的基础状态
    static void fillCommon(const std::shared_ptr<vulkancore::Pipeline> &pipeline);
};
} // namespace VKFW::factories
