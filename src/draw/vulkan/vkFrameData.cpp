#include "vkFrameData.h"

namespace hax {

	namespace draw {

		namespace vk {

			FrameData::FrameData() :
				f{}, hDevice {}, hCommandPool{}, hCommandBuffer{}, hImageView{}, hFrameBuffer{},
				textureBufferBackend{}, solidBufferBackend{}, hFence{} {}


			FrameData::FrameData(FrameData&& fd) noexcept :
				f{ fd.f }, hDevice{ fd.hDevice }, hCommandPool{ fd.hCommandPool }, hCommandBuffer{ fd.hCommandBuffer }, hImageView{ fd.hImageView }, hFrameBuffer{ fd.hFrameBuffer },
				textureBufferBackend{ static_cast<BufferBackend&&>(fd.textureBufferBackend) }, solidBufferBackend{ static_cast<BufferBackend&&>(fd.solidBufferBackend) }, hFence{ fd.hFence } {
				fd.hCommandBuffer = VK_NULL_HANDLE;
				fd.hImageView = VK_NULL_HANDLE;
				fd.hFrameBuffer = VK_NULL_HANDLE;
				fd.hFence = VK_NULL_HANDLE;

				return;
			};


			FrameData::~FrameData() {
				
				if (
					this->f.pVkFreeCommandBuffers && this->f.pVkDestroyFramebuffer && this->f.pVkDestroyImageView &&
					this->f.pVkWaitForFences && this->f.pVkDestroyFence &&
					this->f.pVkUnmapMemory && this->f.pVkFreeMemory && this->f.pVkDestroyBuffer
				) {
					this->destroy();
				}
				
				return;
			}



			bool FrameData::create(
				Functions fn, VkDevice hDev, VkCommandPool hCmdPool, VkImage hImage, VkRenderPass hRenderPass, uint32_t width, uint32_t height,
				VkPhysicalDeviceMemoryProperties memoryProperties, VkPipelineLayout hPipelineLayout, VkPipeline hPipelineTexture, VkPipeline hPipelinePassthrough
			) {
				this->f = fn;
				this->hDevice = hDev;
				this->hCommandPool = hCmdPool;
				
				VkCommandBufferAllocateInfo commandBufferAllocInfo{};
				commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocInfo.commandPool = this->hCommandPool;
				commandBufferAllocInfo.commandBufferCount = 1u;

				if (this->f.pVkAllocateCommandBuffers(this->hDevice, &commandBufferAllocInfo, &this->hCommandBuffer) != VK_SUCCESS) {
					this->destroy();

					return false;
				}

				VkImageSubresourceRange imageSubresourceRange{};
				imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageSubresourceRange.levelCount = 1u;
				imageSubresourceRange.layerCount = 1u;

				VkImageViewCreateInfo imageViewCreateInfo{};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.image = hImage;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
				imageViewCreateInfo.subresourceRange = imageSubresourceRange;

				if (this->f.pVkCreateImageView(this->hDevice, &imageViewCreateInfo, nullptr, &this->hImageView) != VK_SUCCESS) {
					this->destroy();

					return false;
				}

				VkFramebufferCreateInfo framebufferCreateInfo{};
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCreateInfo.renderPass = hRenderPass;
				framebufferCreateInfo.attachmentCount = 1u;
				framebufferCreateInfo.pAttachments = &this->hImageView;
				framebufferCreateInfo.layers = 1u;
				framebufferCreateInfo.width = width;
				framebufferCreateInfo.height = height;

				if (this->f.pVkCreateFramebuffer(this->hDevice, &framebufferCreateInfo, nullptr, &this->hFrameBuffer) != VK_SUCCESS) {
					this->destroy();

					return false;
				}

				constexpr size_t INITIAL_BUFFER_SIZE = 100u;

				this->textureBufferBackend.initialize(this->f, this->hDevice, this->hCommandBuffer, memoryProperties, hPipelineLayout, hPipelineTexture);

				if (!this->textureBufferBackend.create(INITIAL_BUFFER_SIZE)) {
					this->destroy();

					return false;
				}

				this->solidBufferBackend.initialize(this->f, this->hDevice, this->hCommandBuffer, memoryProperties, hPipelineLayout, hPipelinePassthrough);

				if (!this->solidBufferBackend.create(INITIAL_BUFFER_SIZE)) {
					this->destroy();

					return false;
				}

				VkFenceCreateInfo fenceCreateInfo{};
				fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

				if (this->f.pVkCreateFence(this->hDevice, &fenceCreateInfo, nullptr, &this->hFence) != VK_SUCCESS) {
					this->destroy();

					return false;
				}

				return true;
			}


			void FrameData::destroy() {

				if (this->hDevice != VK_NULL_HANDLE && this->hFence != VK_NULL_HANDLE) {
					this->f.pVkWaitForFences(this->hDevice, 1u, &this->hFence, VK_TRUE, UINT64_MAX);
					this->f.pVkDestroyFence(this->hDevice, this->hFence, nullptr);
					this->hFence = VK_NULL_HANDLE;
				}

				if (this->hDevice != VK_NULL_HANDLE && this->hFrameBuffer != VK_NULL_HANDLE) {
					this->f.pVkDestroyFramebuffer(this->hDevice, this->hFrameBuffer, nullptr);
					this->hFrameBuffer = VK_NULL_HANDLE;
				}

				if (this->hDevice != VK_NULL_HANDLE && this->hImageView != VK_NULL_HANDLE) {
					this->f.pVkDestroyImageView(this->hDevice, this->hImageView, nullptr);
					this->hImageView = VK_NULL_HANDLE;
				}

				this->solidBufferBackend.destroy();
				this->textureBufferBackend.destroy();

				if (this->hDevice != VK_NULL_HANDLE && this->hCommandPool != VK_NULL_HANDLE && this->hCommandBuffer != VK_NULL_HANDLE) {
					this->f.pVkFreeCommandBuffers(this->hDevice, this->hCommandPool, 1u, &this->hCommandBuffer);
					this->hCommandBuffer = VK_NULL_HANDLE;
				}

				return;
			}

		}

	}

}