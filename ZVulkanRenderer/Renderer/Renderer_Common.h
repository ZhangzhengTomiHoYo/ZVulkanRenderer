#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 相机 UBO：binding 0，保存观察矩阵和投影矩阵
struct CameraUBO
{
    glm::mat4 view      = glm::mat4(1.0f);
    glm::mat4 proj      = glm::mat4(1.0f);
    glm::vec4 cameraPos = glm::vec4(0.0f);
};

// 物体 UBO：binding 1，保存模型变换矩阵
struct ObjectUBO
{
    glm::mat4 model        = glm::mat4(1.0f);
    glm::mat4 normalMatrix = glm::mat4(1.0f);
};

// 方向光 UBO，位于共享 set 0 的 binding 1。
// direction 为世界空间中光朝场景传播的方向；
// 着色时使用其反方向 L，即从表面指向光源的方向。
// color.rgb 保存光的颜色，color.a 保存强度倍率。
struct LightUBO
{
    glm::vec4 direction{-1.0f, -1.0f, -1.0f, 0.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 2.0f};
};