#pragma once
#include "vkBufferBackend.h"

namespace hax {

	namespace draw {

		namespace vk {

			struct FrameData {
				Functions f;
				VkDevice hDevice;
				VkCommandPool hCommandPool;
				VkCommandBuffer hCommandBuffer;
				VkImageView hImageView;
				VkFramebuffer hFrameBuffer;
				BufferBackend bufferBackend;
				VkFence hFence;

				FrameData();

				FrameData(FrameData&& fd) noexcept;

				FrameData(const FrameData&) = delete;
				FrameData& operator=(FrameData&&) = delete;
				FrameData& operator=(const FrameData&) = delete;

				~FrameData();

				// Creates internal resources.
				//
				// Parameters:
				// 
				// [in] fn
				// Function pointers to the Vulkan functions.
				//
				// [in] pDevice:
				// Logical device of the backend.
				//
				// [in] hCmdPool:
				// Command pool of the backend.
				// 
				// [in] hImage:
				// Image of the swapchain.
				// 
				// [in] hRenderPasst:
				// Current render pass of the backend.
				// 
				// [in] width:
				// Width of the current viewport.
				// 
				// [in] height:
				// Heigth of the current viewport.
				// 
				// [in] memoryProperties:
				// Memory properties of the physical device.
				// 
				// [in] hPipelineLayout:
				// Pipeline layout of the backend.
				//
				// Return:
				// True on success, false on failure.
				bool create(
					Functions fn, VkDevice hDev, VkCommandPool hCmdPool, VkImage hImage, VkRenderPass hRenderPass, uint32_t width, uint32_t height,
					VkPhysicalDeviceMemoryProperties memoryProperties, VkPipelineLayout hPipelineLayout
				);

				// Destroys all internal resources.
				void destroy();
			};

		}

	}

}