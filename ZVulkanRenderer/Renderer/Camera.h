#pragma once
#include "Renderer_Common.h"

namespace VKFW::renderer
{

// 相机支持的移动方向。
// 与窗口层的 CameraMove 分开定义，
// 相机不需要包含窗口层头文件；
// 由 VulkanApp 转换这两种枚举。
enum class CameraMove
{
    Front,
    Back,
    Left,
    Right
};

class Camera
{
  public:
    Camera()  = default;
    ~Camera() = default;

    // 根据目标、距离和观察方向确定相机位置：
    // 相机位置 = 目标位置 - 观察方向 × 距离。
    // 相机沿观察方向看向目标。
    void init(glm::vec3 targetPosition, float distanceFromTarget, glm::vec3 viewDirection);

    // 通过相机位置、目标位置和向上方向设置视角。
    void lookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up);

    void setPerspective(float fovYDegrees, float aspect, float zNear, float zFar);

    // 根据方向移动相机：左右平移，或沿观察方向前进、后退。
    void move(CameraMove mode);

    // 按住鼠标左键拖动时转动视角，由 VulkanApp::onMouseMove 调用。
    void onMouseMove(double xpos, double ypos);
    // 拖动结束时重置状态，避免下次拖动时视角突然跳动。
    void resetMouseState() { mFirstMove = true; }

    // 在 XZ 平面绕目标旋转相机，用于 PBR 展示。
    void horizontalRoundRotate(float dt, glm::vec3 targetPosition, float distanceFromTarget, float rotateSpeedDeg);

    void setSpeed(float speed) { mSpeed = speed; }
    void setSensitivity(float sensitivity) { mSensitivity = sensitivity; }

    glm::mat4 getViewMatrix() const { return mViewMatrix; }
    glm::mat4 getProjectMatrix() const { return mProjMatrix; }
    glm::vec4 getCameraPosition() const { return glm::vec4(mPosition, 1.0f); }

  private:
    void pitch(float yOffset);
    void yaw(float xOffset);

    // 根据水平角和俯仰角重新计算观察方向。
    void updateFrontFromAngles();
    // 根据相机位置、观察方向和向上方向重新计算观察矩阵。
    void update();

  private:
    glm::vec3 mPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 mFront    = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 mUp       = glm::vec3(0.0f, 1.0f, 0.0f);

    // 绕目标旋转所需的数据。
    glm::vec3 mTargetPosition       = glm::vec3(0.0f);
    glm::vec3 mInitialViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    float     mDistanceFromTarget   = 5.0f;
    float     mRotateAngle          = 0.0f; // 累计旋转角度，单位为度

    // 移动速度、俯仰角、水平角和鼠标灵敏度。
    float mSpeed       = 0.01f;
    float mPitch       = 0.0f;
    float mYaw         = -90.0f;
    float mSensitivity = 0.1f;

    // 记录上次鼠标位置和是否刚开始拖动。
    float mLastX     = 0.0f;
    float mLastY     = 0.0f;
    bool  mFirstMove = true;

    glm::mat4 mViewMatrix = glm::mat4(1.0f);
    glm::mat4 mProjMatrix = glm::mat4(1.0f);
};

} // namespace VKFW::renderer
