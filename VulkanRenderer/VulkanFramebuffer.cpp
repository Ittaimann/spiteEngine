#include "VulkanFramebuffer.h"

VulkanFramebuffer::VulkanFramebuffer()
{
}
VulkanFramebuffer::~VulkanFramebuffer()
{
    cleanup();
}

void VulkanFramebuffer::init(VkDevice device, uint32_t width, uint32_t height,const VulkanRenderPass& renderPass,const std::vector<VkImageView> &imageViews)
{
    mDevice = device;
    mAttachments = imageViews;
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags =  0;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass.getRenderPass();
    framebufferInfo.attachmentCount = 2; // TODO: generic this, for now it just depth and color
    framebufferInfo.pAttachments = mAttachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = 1; //Knowledge: how does this matter/where would I use this?

    vkCreateFramebuffer(device,&framebufferInfo,nullptr,&mFramebuffer);
}

void VulkanFramebuffer::cleanup()
{
    if(mFramebuffer != VK_NULL_HANDLE){
        vkDestroyFramebuffer(mDevice,mFramebuffer,nullptr);
        mFramebuffer = VK_NULL_HANDLE;
    }
}