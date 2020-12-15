#include "VulkanBuffer.h"
#include <assert.h>
#include <string.h>
VulkanBuffer::VulkanBuffer()
{
}

VulkanBuffer::~VulkanBuffer()
{
    cleanup();
}

void VulkanBuffer::init(VmaAllocator *allocator, void *data, uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage VmaUsage)
{
    mAllocator = allocator;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VmaUsage;
    allocInfo.pUserData = data;
    VkResult result = vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &mBuffer, &mVmaAlloc, nullptr);
    assert(result == VK_SUCCESS);

    mSize = size;

    //TODO: figure out how you want to handle staging buffers with vma.
    void* mapdata;
    vmaMapMemory(*allocator, mVmaAlloc, &mapdata);
    memcpy(mapdata, data, (size_t)size);
    vmaUnmapMemory(*allocator, mVmaAlloc);
}

void VulkanBuffer::cleanup()
{
    if (mBuffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(*mAllocator, mBuffer, mVmaAlloc);
        mBuffer = VK_NULL_HANDLE;
    }
}

VkBuffer VulkanBuffer::getBuffer()
{
    return mBuffer;
}
