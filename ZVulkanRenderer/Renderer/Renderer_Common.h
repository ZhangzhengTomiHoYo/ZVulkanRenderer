#pragma once
#include <glm/glm.hpp>

// 相机 UBO：binding 0，保存观察矩阵和投影矩阵
struct CameraUBO
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
};

// 物体 UBO：binding 1，保存模型变换矩阵
struct ObjectUBO
{
    glm::mat4 model = glm::mat4(1.0f);
};