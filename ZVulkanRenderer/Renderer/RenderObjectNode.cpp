#include "RenderObjectNode.h"
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <utility>

namespace VKFW::renderer
{

void RenderObjectNode::setPosition(float x, float y, float z)
{
    mPosition       = glm::vec3(x, y, z);
    mTransformDirty = true;
}

void RenderObjectNode::setRotationEuler(float x, float y, float z)
{
    mRotation       = glm::vec3(x, y, z);
    mTransformDirty = true;
}

void RenderObjectNode::setScale(float x, float y, float z)
{
    mScale          = glm::vec3(x, y, z);
    mTransformDirty = true;
}

void RenderObjectNode::updateTransformIfNeeded()
{
    if (!mTransformDirty)
        return;

    // 按平移 × 旋转 × 缩放组合模型矩阵；欧拉角先转换成四元数。
    mModelMatrix = glm::translate(glm::mat4(1.0f), mPosition) * glm::mat4_cast(glm::quat(mRotation)) * glm::scale(glm::mat4(1.0f), mScale);

    mNormalMatrix = glm::transpose(glm::inverse(mModelMatrix));

    mTransformDirty = false;
}

void RenderObjectNode::setShaderFiles(std::string vertexSpv, std::string fragmentSpv)
{
    mVertexShaderFile   = std::move(vertexSpv);
    mFragmentShaderFile = std::move(fragmentSpv);
}

std::vector<VkVertexInputBindingDescription> RenderObjectNode::getVertexInputBindingDescriptions() const
{
    if (mModels.empty() || !mModels[0])
        return {};
    return mModels[0]->getBindingDescriptions();
}

std::vector<VkVertexInputAttributeDescription> RenderObjectNode::getVertexInputAttributeDescriptions() const
{
    if (mModels.empty() || !mModels[0])
        return {};
    return mModels[0]->getAttributeDescriptions();
}

void RenderObjectNode::draw(const std::shared_ptr<vulkancore::CommandBuffer> &commandBuffer)
{
    if (!commandBuffer)
        throw std::runtime_error("RenderObjectNode::draw: command buffer is null");

    updateTransformIfNeeded();

    for (const auto &model : mModels)
    {
        if (model)
            model->draw(commandBuffer);
    }
}

} // namespace VKFW::renderer