#include "VulkanFramebuffer.h"

VulkanFramebuffer::VulkanFramebuffer()
{
}
VulkanFramebuffer::~VulkanFramebuffer()
{
    cleanup();
}

//REFACTOR: this will have to change to reflect the render graph-ish changes
void VulkanFramebuffer::init(VkDevice device, uint32_t width, uint32_t height, const VulkanRenderPass &renderPass, const std::vector<VkImageView> &imageViews)
{
    mDevice = device;
    mAttachments = imageViews;
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags = 0;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass.getRenderPass();
    framebufferInfo.attachmentCount = imageViews.size();
    framebufferInfo.pAttachments = mAttachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1; //KNOWLEDGE: how does this matter/where would I use this?

    mRenderArea.extent = {width, height};
    mRenderArea.offset = {0, 0};
    vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffer);
}

void VulkanFramebuffer::cleanup()
{
    if (mFramebuffer != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(mDevice, mFramebuffer, nullptr);
        mFramebuffer = VK_NULL_HANDLE;
    }
}

VkFramebuffer VulkanFramebuffer::getFramebuffer()
{
    return mFramebuffer;
}

VkRect2D VulkanFramebuffer::getRenderArea()
{
    return mRenderArea;
}