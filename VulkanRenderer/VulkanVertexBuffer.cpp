#include "VulkanVertexBuffer.h"
#include "vk_mem_alloc.h"
#include "modelLoad.h"
VulkanVertexBuffer::VulkanVertexBuffer() {}
VulkanVertexBuffer::~VulkanVertexBuffer() {}
void VulkanVertexBuffer::init(const ModelLoad *model, VmaAllocator *alloc)
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    mBuffer.init(*alloc, (void *)model->getData().data(), model->getSize(), usage, VMA_MEMORY_USAGE_GPU_ONLY);
}
void VulkanVertexBuffer::cleanup() {}