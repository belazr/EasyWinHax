#pragma once
#include "vkDefs.h"
#include "..\AbstractDrawBuffer.h"

namespace hax {

	namespace draw {

		namespace vk {
			
			class DrawBuffer : public AbstractDrawBuffer {
			private:
				Functions _f;
				VkDevice _hDevice;
				VkCommandBuffer _hCommandBuffer;
				VkPipeline _hPipeline;
				VkPhysicalDeviceMemoryProperties _memoryProperties;
				VkBuffer _hVertexBuffer;
				VkBuffer _hIndexBuffer;
				VkDeviceMemory _hVertexMemory;
				VkDeviceMemory _hIndexMemory;
				VkDeviceSize _bufferAlignment;

			public:
				DrawBuffer();

				~DrawBuffer();

				// Initializes members.
				//
				// Parameters:
				// 
				// [in] f:
				// Function pointers to the Vulkan functions.
				// 
				// [in] hDevice:
				// Logical device of the backend.
				// 
				// [in] hCommandBuffer:
				// Command buffer of the image the buffer belongs to.
				// 
				// [in] hPipeline:
				// Pipeline with the appropriate primitive topology of the backend.
				// 
				// [in] memoryProperties:
				// Memory properties of the physical device of the backend.
				void initialize(Functions f, VkDevice hDevice, VkCommandBuffer hCommandBuffer, VkPipeline hPipeline, VkPhysicalDeviceMemoryProperties memoryProperties);

				// Creates a new buffer with all internal resources.
				//
				// Parameters:
				// 
				// [in] vertexCount:
				// Size of the buffer in vertices.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t vertexCount) override;

				// Destroys the buffer and all internal resources.
				void destroy() override;

				// Maps the allocated VRAM into the address space of the current process. Needs to be called before the buffer can be filled.
				//
				// Return:
				// True on success, false on failure.
				bool map() override;

				// Draws the content of the buffer to the screen.
				// Needs to be called between a successful of vk::Backend::beginFrame and a call to vk::Backend::endFrame.
				void draw() override;

			private:
				bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, uint32_t* pSize, VkBufferUsageFlags usage);
				uint32_t getMemoryTypeIndex(uint32_t typeBits) const;

			
			};

		}

	}

}