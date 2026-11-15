#include "Texture.h"
#include <stdexcept>

namespace VKFW::vulkancore
{

std::shared_ptr<Texture> Texture::create(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath)
{
    return std::make_shared<Texture>(device, commandPool, filePath);
}

std::shared_ptr<Texture> Texture::createFromImage(const std::shared_ptr<Device> &device, const std::shared_ptr<Image> &image, const std::shared_ptr<Sampler> &sampler)
{
    return std::make_shared<Texture>(device, image, sampler);
}

Texture::Texture(const std::shared_ptr<Device> &device, const std::shared_ptr<CommandPool> &commandPool, const std::string &filePath) : mDevice(device)
{
    mImage   = Image::createImageFromFile(device, commandPool, filePath);
    mSampler = std::make_shared<Sampler>(device);

    mImageInfo.imageLayout = mImage->getLayout();
    mImageInfo.imageView   = mImage->getImageView();
    mImageInfo.sampler     = mSampler->getSampler();
}

Texture::Texture(const std::shared_ptr<Device> &device, const std::shared_ptr<Image> &image, const std::shared_ptr<Sampler> &sampler) : mDevice(device), mImage(image)
{
    if (!mImage)
        throw std::runtime_error("Texture: image must not be null");

    mSampler = sampler ? sampler : std::make_shared<Sampler>(device);

    mImageInfo.imageLayout = mImage->getLayout();
    mImageInfo.imageView   = mImage->getImageView();
    mImageInfo.sampler     = mSampler->getSampler();
}

} // namespace VKFW::vulkancore