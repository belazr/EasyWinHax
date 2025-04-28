#pragma once
#include "vkDefs.h"
#include "..\IBufferBackend.h"

namespace hax {

	namespace draw {

		namespace vk {

			class BufferBackend : public IBufferBackend {
			private:
				Functions _f;
				VkDevice _hDevice;
				VkCommandBuffer _hCommandBuffer;
				VkPhysicalDeviceMemoryProperties _memoryProperties;
				VkPipelineLayout _hPipelineLayout;
				VkPipeline _hPipeline;
				VkBuffer _hVertexBuffer;
				VkBuffer _hIndexBuffer;
				VkDeviceMemory _hVertexMemory;
				VkDeviceMemory _hIndexMemory;
				VkDeviceSize _bufferAlignment;

				uint32_t _capacity;

			public:
				BufferBackend();

				~BufferBackend();

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
				// [in] memoryProperties:
				// Memory properties of the physical device of the backend.
				//
				// [in] hPipelineLayout:
				// The pipeline layout of the backend.
				// 
				// [in] hPipeline:
				// Pipeline with the appropriate primitive topology.
				void initialize(
					Functions f, VkDevice hDevice, VkCommandBuffer hCommandBuffer,
					VkPhysicalDeviceMemoryProperties memoryProperties, VkPipelineLayout hPipelineLayout, VkPipeline hPipeline
				);

				// Creates internal resources.
				//
				// Parameters:
				//
				// [in] capacity:
				// Capacity of verticies the buffer backend can hold.
				//
				// Return:
				// True on success, false on failure.
				bool create(uint32_t capacity) override;

				// Destroys all internal resources.
				void destroy() override;

				// Gets the current capacity of the buffer in vertices.
				//
				// Return:
				// The current capacity of the buffer in vertices.
				virtual uint32_t capacity() const override;

				// Maps the allocated VRAM into the address space of the current process.
				// 
				// Parameters:
				// 
				// [out] ppLocalVertexBuffer:
				// The mapped vertex buffer.
				//
				// 
				// [out] ppLocalIndexBuffer:
				// The mapped index buffer.
				//
				// Return:
				// True on success, false on failure.
				bool map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) override;

				// Unmaps the allocated VRAM from the address space of the current process.
				virtual void unmap() override;

				// Begins drawing of the content of the buffer. Has to be called before any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual bool begin() override;

				// Draws a batch.
				// 
				// Parameters:
				// 
				// [in] textureId:
				// ID of the texture that should be drawn return by Backend::loadTexture.
				// If this is 0ull, no texture will be drawn.
				//
				// [in] index:
				// Index into the index buffer where the batch begins.
				// 
				// [in] count:
				// Vertex count in the batch.
				virtual void draw(TextureId textureId, uint32_t index, uint32_t count) const override;

				// Ends drawing of the content of the buffer. Has to be called after any draw calls.
				//
				// Return:
				// True on success, false on failure.
				virtual void end() const override;

			private:
				bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, uint32_t* pSize, VkBufferUsageFlags usage);
				uint32_t getMemoryTypeIndex(uint32_t typeBits) const;
			};

		}

	}

}