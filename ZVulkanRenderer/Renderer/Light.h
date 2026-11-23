#pragma once
#include "../Common.h"
#include "Renderer_Common.h"

namespace VKFW::renderer
{

class Light
{
  public:
    Light()  = default;
    ~Light() = default;

    // 光朝场景传播的方向，设置时归一化。
    void setDirection(const glm::vec3 &dir) { mDirection = glm::normalize(dir); }
    void setColor(const glm::vec3 &color) { mColor = color; }
    void setIntensity(float intensity) { mIntensity = intensity; }

    glm::vec3 getDirection() const { return mDirection; }
    glm::vec3 getColor() const { return mColor; }
    float     getIntensity() const { return mIntensity; }

    // 打包成着色器读取的 UBO。
    LightUBO getUBO() const
    {
        LightUBO ubo{};
        ubo.direction = glm::vec4(mDirection, 0.0f);
        ubo.color     = glm::vec4(mColor, mIntensity);
        return ubo;
    }

  private:
    glm::vec3 mDirection{-1.0f, -1.0f, -1.0f}; // 光朝场景传播的方向。
    glm::vec3 mColor{1.0f, 1.0f, 1.0f};
    float     mIntensity{2.0f};                // 默认灯光强度倍率为 2.0
};

} // namespace VKFW::renderer