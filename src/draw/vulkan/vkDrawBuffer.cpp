#include "vkDrawBuffer.h"

namespace hax{

	namespace draw {

		namespace vk {

			DrawBuffer::DrawBuffer() :
				_f{}, _hDevice{}, _hCommandBuffer{}, _hPipeline{}, _memoryProperties{},
				_hVertexBuffer{}, _hIndexBuffer{}, _hVertexMemory{}, _hIndexMemory{}, _bufferAlignment{ 0x10u } {}
			
			
			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(Functions f, VkDevice hDevice, VkCommandBuffer hCommandBuffer, VkPipeline hPipeline, VkPhysicalDeviceMemoryProperties memoryProperties) {
				this->_f = f;
				this->_hDevice = hDevice;
				this->_hCommandBuffer = hCommandBuffer;
				this->_hPipeline = hPipeline;
				this->_memoryProperties = memoryProperties;

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				memset(this, 0, sizeof(DrawBuffer));

				uint32_t newVertexBufferSize = vertexCount * sizeof(Vertex);

				if (!this->createBuffer(&this->_hVertexBuffer, &this->_hVertexMemory, &newVertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

				this->vertexBufferSize = newVertexBufferSize;

				uint32_t newIndexBufferSize = vertexCount * sizeof(uint32_t);

				if (!this->createBuffer(&this->_hIndexBuffer, &this->_hIndexMemory, &newIndexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

				this->indexBufferSize = newIndexBufferSize;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_hIndexMemory != VK_NULL_HANDLE) {
					this->_f.pVkUnmapMemory(this->_hDevice, this->_hIndexMemory);
					this->_f.pVkFreeMemory(this->_hDevice, this->_hIndexMemory, nullptr);
					this->_hIndexMemory = VK_NULL_HANDLE;
				}

				if (this->_hIndexBuffer != VK_NULL_HANDLE) {
					this->_f.pVkDestroyBuffer(this->_hDevice, this->_hIndexBuffer, nullptr);
					this->_hIndexBuffer = VK_NULL_HANDLE;
				}

				if (this->_hVertexMemory != VK_NULL_HANDLE) {
					this->_f.pVkUnmapMemory(this->_hDevice, this->_hVertexMemory);
					this->_f.pVkFreeMemory(this->_hDevice, this->_hVertexMemory, nullptr);
					this->_hVertexMemory = VK_NULL_HANDLE;
				}

				if (this->_hVertexBuffer != VK_NULL_HANDLE) {
					this->_f.pVkDestroyBuffer(this->_hDevice, this->_hVertexBuffer, nullptr);
					this->_hVertexBuffer = VK_NULL_HANDLE;
				}

				this->pLocalVertexBuffer = nullptr;
				this->pLocalIndexBuffer = nullptr;
				this->vertexBufferSize = 0u;
				this->indexBufferSize = 0u;
				this->curOffset = 0u;

				return;
			}


			bool DrawBuffer::map() {

				if (!this->pLocalVertexBuffer) {

					if (this->_f.pVkMapMemory(this->_hDevice, this->_hVertexMemory, 0ull, this->vertexBufferSize, 0ull, reinterpret_cast<void**>(&this->pLocalVertexBuffer)) != VkResult::VK_SUCCESS) return false;

				}

				if (!this->pLocalIndexBuffer) {

					if (this->_f.pVkMapMemory(this->_hDevice, this->_hIndexMemory, 0ull, this->indexBufferSize, 0ull, reinterpret_cast<void**>(&this->pLocalIndexBuffer)) != VkResult::VK_SUCCESS) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				VkMappedMemoryRange ranges[2]{};
				ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				ranges[0].memory = this->_hVertexMemory;
				ranges[0].size = VK_WHOLE_SIZE;
				ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				ranges[1].memory = this->_hIndexMemory;
				ranges[1].size = VK_WHOLE_SIZE;

				if (this->_f.pVkFlushMappedMemoryRanges(this->_hDevice, _countof(ranges), ranges) != VkResult::VK_SUCCESS) return;

				this->_f.pVkUnmapMemory(this->_hDevice, this->_hVertexMemory);
				this->pLocalVertexBuffer = nullptr;

				this->_f.pVkUnmapMemory(this->_hDevice, this->_hIndexMemory);
				this->pLocalIndexBuffer = nullptr;

				constexpr VkDeviceSize offset = 0ull;
				this->_f.pVkCmdBindPipeline(this->_hCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hPipeline);
				this->_f.pVkCmdBindVertexBuffers(this->_hCommandBuffer, 0u, 1u, &this->_hVertexBuffer, &offset);
				this->_f.pVkCmdBindIndexBuffer(this->_hCommandBuffer, this->_hIndexBuffer, 0ull, VK_INDEX_TYPE_UINT32);
				this->_f.pVkCmdDrawIndexed(this->_hCommandBuffer, this->curOffset, 1u, 0u, 0u, 0u);

				this->curOffset = 0u;

				return;
			}


			bool DrawBuffer::createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, uint32_t* pSize, VkBufferUsageFlagBits usage) {
				const VkDeviceSize sizeAligned = (((*pSize) - 1ul) / this->_bufferAlignment + 1ul) * this->_bufferAlignment;

				VkBufferCreateInfo bufferCreateinfo{};
				bufferCreateinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateinfo.size = sizeAligned;
				bufferCreateinfo.usage = usage;
				bufferCreateinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				if (!this->_f.pVkCreateBuffer(this->_hDevice, &bufferCreateinfo, nullptr, phBuffer) == VkResult::VK_SUCCESS) return false;

				VkMemoryRequirements memoryReq{};
				this->_f.pVkGetBufferMemoryRequirements(this->_hDevice, *phBuffer, &memoryReq);
				this->_bufferAlignment = memoryReq.alignment;

				VkMemoryAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				allocInfo.allocationSize = memoryReq.size;
				allocInfo.memoryTypeIndex = this->getMemoryTypeIndex(memoryReq.memoryTypeBits);

				if (!this->_f.pVkAllocateMemory(this->_hDevice, &allocInfo, nullptr, phMemory) == VkResult::VK_SUCCESS) return false;

				if (!this->_f.pVkBindBufferMemory(this->_hDevice, *phBuffer, *phMemory, 0) == VkResult::VK_SUCCESS) return false;

				*pSize = static_cast<uint32_t>(memoryReq.size);

				return true;
			}


			uint32_t DrawBuffer::getMemoryTypeIndex(uint32_t typeBits) const {

				for (uint32_t i = 0u; i < this->_memoryProperties.memoryTypeCount; i++) {

					if ((this->_memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && typeBits & (1 << i)) {
						return i;
					}

				}

				return 0xFFFFFFFF;
			}

		}

	}

}