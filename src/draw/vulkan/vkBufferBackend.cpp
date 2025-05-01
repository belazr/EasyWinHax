#include "vkBufferBackend.h"
#include <stdlib.h>

namespace hax {

	namespace draw {

		namespace vk {

			BufferBackend::BufferBackend() :
				_f{}, _hDevice{}, _hCommandBuffer{}, _memoryProperties{}, _hPipelineLayout{}, _hPipeline{},
				_hVertexBuffer{}, _hIndexBuffer{}, _hVertexMemory{}, _hIndexMemory{}, _bufferAlignment{ 0x10u }, _capacity{} {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(
				Functions f, VkDevice hDevice, VkCommandBuffer hCommandBuffer,
				VkPhysicalDeviceMemoryProperties memoryProperties, VkPipelineLayout hPipelineLayout, VkPipeline hPipeline
			) {
				this->_f = f;
				this->_hDevice = hDevice;
				this->_hCommandBuffer = hCommandBuffer;
				this->_memoryProperties = memoryProperties;
				this->_hPipelineLayout = hPipelineLayout;
				this->_hPipeline = hPipeline;

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				
				if (this->_capacity) return false;
				
				uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(&this->_hVertexBuffer, &this->_hVertexMemory, &vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
					this->destroy();

					return false;
				}

				uint32_t indexBufferSize = capacity * sizeof(uint32_t);

				if (!this->createBuffer(&this->_hIndexBuffer, &this->_hIndexMemory, &indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) {
					this->destroy();

					return false;
				}

				this->_capacity = capacity;

				return true;
			}


			void BufferBackend::destroy() {

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

				this->_capacity = 0u;

				return;
			}


			uint32_t BufferBackend::capacity() const {

				return this->_capacity;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				if (this->_f.pVkMapMemory(this->_hDevice, this->_hVertexMemory, 0ull, this->_capacity * sizeof(Vertex), 0ull, reinterpret_cast<void**>(ppLocalVertexBuffer)) != VK_SUCCESS) {
					this->unmap();

					return false;
				}

				if (this->_f.pVkMapMemory(this->_hDevice, this->_hIndexMemory, 0ull, this->_capacity * sizeof(uint32_t), 0ull, reinterpret_cast<void**>(ppLocalIndexBuffer)) != VK_SUCCESS) {
					this->unmap();

					return false;
				}

				return true;
			}


			void BufferBackend::unmap() {
				VkMappedMemoryRange ranges[2]{};
				ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				ranges[0].memory = this->_hIndexMemory;
				ranges[0].size = VK_WHOLE_SIZE;
				ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				ranges[1].memory = this->_hVertexMemory;
				ranges[1].size = VK_WHOLE_SIZE;

				this->_f.pVkFlushMappedMemoryRanges(this->_hDevice, _countof(ranges), ranges);

				this->_f.pVkUnmapMemory(this->_hDevice, this->_hIndexMemory);
				this->_f.pVkUnmapMemory(this->_hDevice, this->_hVertexMemory);

				return;
			}


			bool BufferBackend::begin() {
				this->_f.pVkCmdBindPipeline(this->_hCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hPipeline);

				constexpr VkDeviceSize OFFSET = 0ull;
				this->_f.pVkCmdBindVertexBuffers(this->_hCommandBuffer, 0u, 1u, &this->_hVertexBuffer, &OFFSET);
				this->_f.pVkCmdBindIndexBuffer(this->_hCommandBuffer, this->_hIndexBuffer, 0ull, VK_INDEX_TYPE_UINT32);

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) const {

				if (textureId) {
					this->_f.pVkCmdBindDescriptorSets(this->_hCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hPipelineLayout, 0u, 1u, reinterpret_cast<VkDescriptorSet*>(&textureId), 0u, nullptr);
				}
				
				this->_f.pVkCmdDrawIndexed(this->_hCommandBuffer, count, 1u, index, 0u, 0u);

				return;
			}


			void BufferBackend::end() const {

				return;
			}


			bool BufferBackend::createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, uint32_t* pSize, VkBufferUsageFlags usage) {
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


			uint32_t BufferBackend::getMemoryTypeIndex(uint32_t typeBits) const {

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