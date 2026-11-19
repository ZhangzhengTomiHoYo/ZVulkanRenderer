#pragma once
#include "DescriptorManager.h"
#include "Model.h"
#include "Renderer_Common.h"
#include <memory>
#include <string>
#include <vector>

namespace VKFW::renderer
{

// 节点管理 set 1：物体 UBO 和材质纹理。
// set 0 的相机等共享数据由 Scene 管理。
class RenderObjectNode
{
  public:
    RenderObjectNode()  = default;
    ~RenderObjectNode() = default;

    // 位置、旋转和缩放。
    void setPosition(float x, float y, float z);
    void setRotationEuler(float x, float y, float z); // 弧度
    void setScale(float x, float y, float z);

    // 变换发生变化时，重新计算模型矩阵和法线矩阵。
    void             updateTransformIfNeeded();
    const glm::mat4 &getModelMatrix() const { return mModelMatrix; }
    const glm::mat4 &getNormalMatrix() const { return mNormalMatrix; }

    // 提供着色器文件和顶点输入描述，供创建管线使用。
    void               setShaderFiles(std::string vertexSpv, std::string fragmentSpv);
    const std::string &getVertexShaderFile() const { return mVertexShaderFile; }
    const std::string &getFragmentShaderFile() const { return mFragmentShaderFile; }

    // 使用第一个模型的顶点输入描述。
    std::vector<VkVertexInputBindingDescription>   getVertexInputBindingDescriptions() const;
    std::vector<VkVertexInputAttributeDescription> getVertexInputAttributeDescriptions() const;

    // 绘制节点中的模型。
    // 调用前需要绑定场景 set 0 和当前节点 set 1。
    void draw(const std::shared_ptr<vulkancore::CommandBuffer> &commandBuffer);

  public:
    std::vector<std::shared_ptr<Model>> mModels{};

    // set 1：物体 UBO 和纹理，由 Scene 初始化。
    std::shared_ptr<DescriptorManager> mDescriptorManager = nullptr;

  private:
    glm::vec3 mPosition = glm::vec3(0.0f);
    glm::vec3 mRotation = glm::vec3(0.0f); // 欧拉角，单位为弧度
    glm::vec3 mScale    = glm::vec3(1.0f);

    glm::mat4 mModelMatrix    = glm::mat4(1.0f);
    glm::mat4 mNormalMatrix   = glm::mat4(1.0f);
    bool      mTransformDirty = true;

    std::string mVertexShaderFile{};
    std::string mFragmentShaderFile{};
};

} // namespace VKFW::renderer