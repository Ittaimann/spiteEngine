#define GLFW_INCLUDE_VULKAN // uh maybe a different place?
#include "VulkanRenderer.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "Core/ShaderLoad.h"
#include "Core/WindowManager.h"

#include "VulkanFramebuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanVertexBuffer.h"

// TODO:remove this, temporary
#include <glm/vec3.hpp>

VulkanRenderer::VulkanRenderer() {}
VulkanRenderer::~VulkanRenderer() {}

void VulkanRenderer::init(bool validation, WindowManager *window) {
    mInstance.init(validation);
    mSurface.init(mInstance.getInstance(), window->getWindow());
    mPhysicalDevice.init(mInstance.getInstance(), mSurface.getVulkanSurface());
    VulkanHelper::QueueFamilyIndices queueFamilies =
        mPhysicalDevice.getFamilyIndices();
    mDevice.init(validation, mPhysicalDevice.getPhysicalDevice(),
                 queueFamilies);
    mPresentQueue.init(mDevice.getDevice(),
                       queueFamilies.presentFamily.value());
    mGraphicsQueue.init(mDevice.getDevice(),
                        queueFamilies.graphicsFamily.value());
    mCommandPool.init(mDevice.getDevice(),
                      queueFamilies.graphicsFamily.value());
    mSwapChain.init(mDevice.getDevice(), mSurface.getVulkanSurface(),
                    mPhysicalDevice.getSwapChainDetails(), queueFamilies,
                    window->getWidth(), window->getHeight());

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mPhysicalDevice.getPhysicalDevice();
    allocatorInfo.device = mDevice.getDevice();
    allocatorInfo.instance = mInstance.getInstance();

    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    VkSemaphoreCreateInfo semaphoreInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0};
    vkCreateSemaphore(mDevice.getDevice(), &semaphoreInfo, nullptr,
                      &mQueueSubmitSemaphore);

    // TODO: break this off into another function or structure somewhere.
    VkDescriptorPoolSize descriptorPoolSizeInfo = {};
    descriptorPoolSizeInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSizeInfo.descriptorCount = 1;

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.flags = 0;
    descriptorPoolInfo.pPoolSizes = &descriptorPoolSizeInfo;
    descriptorPoolInfo.poolSizeCount = 1;
    descriptorPoolInfo.maxSets = 65535; // TODO: find programmitc way to do
                                        // this.
    // TODO: wrap this in an assert
    vkCreateDescriptorPool(mDevice.getDevice(), &descriptorPoolInfo, nullptr,
                           &mDescriptorPool);

    mDescriptorSet = VK_NULL_HANDLE;
    mCurrentFrame = 0;
    initFrontBuffer();
}

void VulkanRenderer::initFrontBuffer() {
    std::vector<VkImageView> swapChainImageViews = mSwapChain.getImageViews();
    mFrontFrameBuffers.resize(swapChainImageViews.size());
    VkExtent2D swapChainExtent = mSwapChain.getSwapChainExtent();
    mFrontRenderPass.init(mDevice.getDevice());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::vector<VkImageView> indivdualImage = {swapChainImageViews[i]};
        mFrontFrameBuffers.at(i).init(
            mDevice.getDevice(), swapChainExtent.width, swapChainExtent.height,
            mFrontRenderPass, indivdualImage);
    }
}

void VulkanRenderer::cleanup() {
    mFrontFrameBuffers.clear();
    mFrontRenderPass.cleanup();
    vkDestroySemaphore(mDevice.getDevice(), mQueueSubmitSemaphore, nullptr);

    vkDestroyDescriptorSetLayout(mDevice.getDevice(), mDescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(mDevice.getDevice(), mDescriptorPool, nullptr);

    mCommandPool.cleanup(mDevice.getDevice());
    mSwapChain.cleanup(mDevice.getDevice());
    vmaDestroyAllocator(mAllocator);
    mDevice.cleanup();
    mSurface.cleanup(mInstance.getInstance());
    mInstance.cleanup();
}

void VulkanRenderer::buildModel(VulkanVertexBuffer &vertexBuffer,
                                ModelLoad *model, bool deviceLocal) {
    vertexBuffer.init(model, &mAllocator, deviceLocal);
    if (deviceLocal) {
        dataTransfer transfer{vertexBuffer.getStagingPtr(),
                              vertexBuffer.getBufferPtr(), nullptr};
        mCopyCommandQueue.push_back(transfer);
    }
    // TODO: models have to do a lot of extra stuff like cache vertex binding
    // and attribute binding.
}

// TODO: expand this drastically, this will probably be a large amount of the
// texture builds
void VulkanRenderer::buildImage(VulkanImage &image, uint32_t width,
                                uint32_t height, VkFormat format,
                                VkImageUsageFlagBits usage,
                                VkImageAspectFlags imageViewAspect) {
    image.init(&mAllocator, mDevice.getDevice(), width, height, format, usage,
               imageViewAspect);
}

void VulkanRenderer::buildRenderPass(VulkanRenderPass &renderpass) {
    renderpass.init(mDevice.getDevice());
}

void VulkanRenderer::buildFramebuffer(VulkanFramebuffer &framebuffer,
                                      uint32_t width, uint32_t height,
                                      const VulkanRenderPass &renderpass,
                                      const VulkanImage &bufferAttach) {
    std::vector<VkImageView> imageViews = {bufferAttach.getImageView()};
    framebuffer.init(mDevice.getDevice(), width, height, renderpass,
                     imageViews);
}

VulkanFramebuffer *VulkanRenderer::getFrontBuffer() {
    return &mFrontFrameBuffers.at(mCurrentFrame);
}
VulkanRenderPass *VulkanRenderer::getFrontRenderPass() {
    return &mFrontRenderPass;
}

void VulkanRenderer::buildPipeline(VulkanGraphicsPipeline &pipeline,
                                   const VulkanRenderPass &renderpass,
                                   const std::vector<VulkanShader> &shaders) {
    // TODO: figure out how we are dealing with the descriptor layout
    pipeline.init(mDevice.getDevice(), renderpass.getRenderPass(), shaders,
                  mDescriptorSetLayout);
}

void VulkanRenderer::buildShader(VulkanShader &shader, ShaderLoad *shaderText) {
    shader.init(mDevice.getDevice(), shaderText->getSize(),
                shaderText->getData());
}

void VulkanRenderer::buildBuffer(VulkanBuffer &buffer, size_t size,
                                 VkBufferUsageFlags tempUsageFlag, void *data) {
    buffer.init(&mAllocator, data, size, tempUsageFlag,
                VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void VulkanRenderer::buildDescriptorSet(uint32_t bufferDescNum) {
    // build layout binding
    // TODO: this is very important so maybe just like care
    VkDescriptorSetLayoutBinding layoutBindingInfo = {};
    layoutBindingInfo.binding = 0;
    layoutBindingInfo.descriptorCount = 1;
    layoutBindingInfo.pImmutableSamplers = nullptr;
    layoutBindingInfo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindingInfo.stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT; // KNOWLEDGE: double check if this is
                                      // necessary;

    // build layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.pNext = nullptr;
    layoutInfo.flags = 0;
    layoutInfo.bindingCount = bufferDescNum;
    layoutInfo.pBindings = &layoutBindingInfo;

    vkCreateDescriptorSetLayout(mDevice.getDevice(), &layoutInfo, nullptr,
                                &mDescriptorSetLayout);

    // build descriptor
    VkDescriptorSetAllocateInfo descriptorAllocInfo = {};
    descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocInfo.pNext = nullptr;
    descriptorAllocInfo.descriptorPool = mDescriptorPool;
    descriptorAllocInfo.descriptorSetCount = 1;
    descriptorAllocInfo.pSetLayouts = &mDescriptorSetLayout;

    vkAllocateDescriptorSets(mDevice.getDevice(), &descriptorAllocInfo,
                             &mDescriptorSet);
}
// TODO: maybe change this to begin and end Command Recording. makes it more
// obvious this is  command buffer.s
void VulkanRenderer::beginRecording() {
    mCommandPool
        .beginRecording(); // maybe return a handle to the command buffer?
}
void VulkanRenderer::endRecording() { mCommandPool.endRecording(); }

// TODO: ties renderpass and framebuffer together
void VulkanRenderer::beginRenderPass(VulkanRenderPass &renderpass,
                                     VulkanFramebuffer &framebuffer) {
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.renderPass = renderpass.getRenderPass();
    renderPassBeginInfo.framebuffer = framebuffer.getFramebuffer();
    renderPassBeginInfo.renderArea =
        framebuffer
            .getRenderArea(); // this could probably be set here instead
                              // of from the framebuffer but do that later
    renderPassBeginInfo.clearValueCount =
        2; // TODO: temporary for one color and one depth attach;
    VkClearValue clears[2];
    renderPassBeginInfo.pClearValues = clears;

    vkCmdBeginRenderPass(mCommandPool.getCommandBuffer(), &renderPassBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::endRenderPass() {
    vkCmdEndRenderPass(mCommandPool.getCommandBuffer());
}

void VulkanRenderer::bindVertexBuffer(VulkanVertexBuffer &vertexBuffer) {
    VkDeviceSize offsets = 0;
    VkBuffer baseBuffer = vertexBuffer.getVkBuffer();
    vkCmdBindVertexBuffers(mCommandPool.getCommandBuffer(), 0, 1, &baseBuffer,
                           &offsets);
}

void VulkanRenderer::bindPipeline(VulkanGraphicsPipeline &pipeline) {
    vkCmdBindPipeline(mCommandPool.getCommandBuffer(),
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline.getGraphicsPipeline());
}

void VulkanRenderer::bindDescriptorSet(VulkanGraphicsPipeline &pipeline) {
    vkCmdBindDescriptorSets(
        mCommandPool.getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.getGraphicsPipelineLayout(), 0, 1, &mDescriptorSet, 0, 0);
}
void VulkanRenderer::updateDescriptors(uint32_t descriptorWriteCount,
                                       VulkanBuffer *bufferInput) {
    VkDescriptorBufferInfo bufferWriteInfo = {};
    bufferWriteInfo.buffer = bufferInput->getBuffer();
    bufferWriteInfo.offset = 0;
    bufferWriteInfo.range = sizeof(glm::vec3);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = mDescriptorSet;
    descriptorWrite.descriptorCount = descriptorWriteCount;
    descriptorWrite.pBufferInfo = &bufferWriteInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    vkUpdateDescriptorSets(mDevice.getDevice(), descriptorWriteCount,
                           &descriptorWrite, 0, nullptr);
}
void VulkanRenderer::draw() {
    vkCmdDraw(mCommandPool.getCommandBuffer(), 3, 1, 0, 0);
}

void VulkanRenderer::beginFrame() {
    mSwapChain.acquireImageIndex(mDevice.getDevice());
    mCommandPool.beginRecording();
    VkCommandBuffer copyBuffer = mCommandPool.getCommandBuffer();
    for (size_t i = 0; i < mCopyCommandQueue.size(); i++) {
        // TODO: create a path for images.
        mCopyCommandQueue.at(i).dst->unstageBuffer(
            copyBuffer, *mCopyCommandQueue.at(i).src);
    }
    mCommandPool.endRecording();
}

void VulkanRenderer::endFrame() {
    submitFrame();
    presentFrame();
    mCommandPool.endFrame();

    // clear the staging buffers from the frame.
    for (size_t i = 0; i < mCopyCommandQueue.size(); i++) {
        mCopyCommandQueue.at(i).src->cleanup();
    }
    mCopyCommandQueue.clear();
    mCurrentFrame = (mCurrentFrame + 1) %
                    3; // TODO: figure out the proper max frames in flight
}

void VulkanRenderer::submitFrame() {
    std::vector<VkCommandBuffer> submissionBuffers =
        mCommandPool.getUsedCommandBuffers();
    // KNOWLEDGE: look into this a tad bit more. This seems like it could be
    // omega important.
    VkSemaphore waitSemaphore[] = {mSwapChain.getSwapChainSemaphore()};
    VkSemaphore signalSemaphore[] = {mQueueSubmitSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = submissionBuffers.size();
    submitInfo.pCommandBuffers = submissionBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphore;
    VkResult result = vkQueueSubmit(mGraphicsQueue.getQueue(), 1, &submitInfo,
                                    VK_NULL_HANDLE);
    assert(result == VK_SUCCESS);
}

void VulkanRenderer::presentFrame() {
    // TODO: make sure this is set up properly
    VkSemaphore signalSemaphore[] = {mQueueSubmitSemaphore};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphore;
    VkSwapchainKHR swapChains[] = {mSwapChain.getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = mSwapChain.getNextImageIndex();
    presentInfo.pResults = nullptr; // Optional
    vkQueuePresentKHR(mPresentQueue.getQueue(), &presentInfo);
    // TODO: do proper sync other wise this is going to be the noose
    vkDeviceWaitIdle(mDevice.getDevice());
}
