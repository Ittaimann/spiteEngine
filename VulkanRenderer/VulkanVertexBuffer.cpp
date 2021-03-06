#include "VulkanVertexBuffer.h"
#include "vk_mem_alloc.h"
#include "Core/ModelLoad.h"
VulkanVertexBuffer::VulkanVertexBuffer() {}
VulkanVertexBuffer::~VulkanVertexBuffer() {}
//TODO: double check some of this staging stuff. These flags don't look correct to me:
void VulkanVertexBuffer::init(const ModelLoad *model, VmaAllocator *alloc, bool staging)
{
    if (staging)
    {
        mStaging.init(alloc, (void *)model->getData().data(), model->getSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        mBuffer.init(alloc, nullptr, model->getSize(), usage, VMA_MEMORY_USAGE_GPU_ONLY);
    }
    else
    {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        mBuffer.init(alloc, (void *)model->getData().data(), model->getSize(), usage, VMA_MEMORY_USAGE_CPU_ONLY); // this is going to only be used on cpu side.
    }
}
void VulkanVertexBuffer::cleanup() {}

VkBuffer VulkanVertexBuffer::getVkBuffer()
{
    return mBuffer.getBuffer();
}

VulkanBuffer VulkanVertexBuffer::getBuffer()
{
    return mBuffer;
}

//TODO: reconsider this design decision. seems like i'm fighiting c++ but am I winning?
// seems like i'm fighting my cleanup habits which I will nedd to alter immensly.
VulkanBuffer *VulkanVertexBuffer::getBufferPtr()
{
    return &mBuffer;
}
VulkanBuffer* VulkanVertexBuffer::getStagingPtr()
{
    return &mStaging;
}
