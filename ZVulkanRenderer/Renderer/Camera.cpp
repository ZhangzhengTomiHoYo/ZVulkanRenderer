#include "Camera.h"

namespace VKFW::renderer
{

void Camera::init(glm::vec3 targetPosition, float distanceFromTarget, glm::vec3 viewDirection)
{
    viewDirection = glm::normalize(viewDirection);

    mTargetPosition       = targetPosition;
    mInitialViewDirection = viewDirection;
    mDistanceFromTarget   = distanceFromTarget;
    mRotateAngle          = 0.0f;

    mPosition = targetPosition - viewDirection * distanceFromTarget;
    mFront    = viewDirection;

    glm::vec3 right = glm::normalize(glm::cross(mFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    mUp             = glm::normalize(glm::cross(right, mFront));

    // 根据当前观察方向同步俯仰角和水平角，避免鼠标接管时视角跳变。
    mPitch = glm::degrees(asin(mFront.y));
    mYaw   = glm::degrees(atan2(mFront.z, mFront.x));

    update();
}

void Camera::lookAt(glm::vec3 position, glm::vec3 target, glm::vec3 up)
{
    mPosition = position;
    mFront    = glm::normalize(target - position);

    glm::vec3 right = glm::normalize(glm::cross(mFront, up));
    mUp             = glm::normalize(glm::cross(right, mFront));

    mPitch = glm::degrees(asin(mFront.y));
    mYaw   = glm::degrees(atan2(mFront.z, mFront.x));

    update();
}

void Camera::setPerspective(float fovYDegrees, float aspect, float zNear, float zFar)
{
    mProjMatrix = glm::perspective(glm::radians(fovYDegrees), aspect, zNear, zFar);
}

void Camera::move(CameraMove mode)
{
    switch (mode)
    {
    case CameraMove::Left:
        mPosition -= glm::normalize(glm::cross(mFront, mUp)) * mSpeed;
        break;
    case CameraMove::Right:
        mPosition += glm::normalize(glm::cross(mFront, mUp)) * mSpeed;
        break;
    case CameraMove::Front:
        mPosition += mFront * mSpeed;
        break;
    case CameraMove::Back:
        mPosition -= mFront * mSpeed;
        break;
    default:
        break;
    }
    update();
}

void Camera::onMouseMove(double xpos, double ypos)
{
    if (mFirstMove)
    {
        mLastX     = static_cast<float>(xpos);
        mLastY     = static_cast<float>(ypos);
        mFirstMove = false;
        return;
    }

    float xOffset = static_cast<float>(xpos) - mLastX;
    float yOffset = -(static_cast<float>(ypos) - mLastY); // 屏幕 Y 坐标向下增加，这里取反

    mLastX = static_cast<float>(xpos);
    mLastY = static_cast<float>(ypos);

    yaw(xOffset);
    pitch(yOffset);
}

void Camera::horizontalRoundRotate(float dt, glm::vec3 targetPosition, float distanceFromTarget, float rotateSpeedDeg)
{

    mTargetPosition = targetPosition;
    mRotateAngle += rotateSpeedDeg * dt;

    glm::mat4 rotateMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(mRotateAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 viewDirection = glm::normalize(glm::vec3(rotateMatrix * glm::vec4(mInitialViewDirection, 0.0f)));

    mPosition = targetPosition - viewDirection * distanceFromTarget;
    mFront    = viewDirection;

    glm::vec3 right = glm::normalize(glm::cross(mFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    mUp             = glm::normalize(glm::cross(right, mFront));

    update();
}

void Camera::pitch(float yOffset)
{
    mPitch += yOffset * mSensitivity;

    // 限制俯仰角，避免向正上方或正下方看时视角翻转。
    if (mPitch >= 89.0f)
        mPitch = 89.0f;
    if (mPitch <= -89.0f)
        mPitch = -89.0f;

    updateFrontFromAngles();
}

void Camera::yaw(float xOffset)
{
    mYaw += xOffset * mSensitivity;
    updateFrontFromAngles();
}

void Camera::updateFrontFromAngles()
{
    mFront.y = sin(glm::radians(mPitch));
    mFront.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
    mFront   = glm::normalize(mFront);
    update();
}

void Camera::update()
{
    mViewMatrix = glm::lookAt(mPosition, mPosition + mFront, mUp);
}

} // namespace VKFW::renderer
