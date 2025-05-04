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
				BufferBackend triangleListBuffer;
				BufferBackend pointListBuffer;
				BufferBackend textureTriangleListBuffer;
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
				// [in] pDevice:
				// Logical device of the backend.
				//
				// [in] pCommandList:
				// Command list of the backend.
				//
				// [in] pTriangleListPipelineStatePassthrough:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				// 
				// [in] pPointListPipelineStatePassthrough:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				// 
				// [in] pTriangleListPipelineStateTexture:
				// Pipeline state with primitive topology for triangles and a passthrough pixel shader.
				//
				// Return:
				// True on success, false on failure.
				bool create(
					Functions fn, VkDevice hDev, VkCommandPool hCmdPool, VkImage hImage, VkRenderPass hRenderPass, uint32_t width, uint32_t height, VkPhysicalDeviceMemoryProperties memoryProperties,
					VkPipelineLayout hPipelineLayout, VkPipeline hTriangleListPipelinePassthrough, VkPipeline hPointListPipelinePassthrough, VkPipeline hTriangleListPipelineTexture
				);

				// Destroys all internal resources.
				void destroy();
			};

		}

	}

}