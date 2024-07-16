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
				DrawBuffer(Functions f, VkDevice _hDevice, VkCommandBuffer hCommandBuffer, VkPipeline hPipeline, VkPhysicalDeviceMemoryProperties memoryProperties);
				~DrawBuffer();

				bool create(uint32_t vertexCount) override;
				void destroy() override;
				bool map() override;
				void draw() override;

			private:
				bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, uint32_t* pSize, VkBufferUsageFlagBits usage);
				uint32_t getMemoryTypeIndex(uint32_t typeBits) const;

			
			};

		}

	}

}