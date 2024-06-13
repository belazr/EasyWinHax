#pragma once
#include "..\Vertex.h"
#include "..\IDraw.h"

#define VK_NO_PROTOTYPES
#include "include\vulkan.h"

namespace hax {
	
	namespace vk {

		typedef struct VulkanInitData {
			PFN_vkQueuePresentKHR pVkQueuePresentKHR;
			VkDevice hDevice;
		}VulkanInitData;

		bool getVulkanInitData(VulkanInitData* initData);

		class Draw : public IDraw {
		private:
			typedef struct Functions {
				PFN_vkGetPhysicalDeviceMemoryProperties pVkGetPhysicalDeviceMemoryProperties;
				PFN_vkGetSwapchainImagesKHR pVkGetSwapchainImagesKHR;
				PFN_vkCreateCommandPool pVkCreateCommandPool;
				PFN_vkDestroyCommandPool pVkDestroyCommandPool;
				PFN_vkAllocateCommandBuffers pVkAllocateCommandBuffers;
				PFN_vkFreeCommandBuffers pVkFreeCommandBuffers;
				PFN_vkCreateImageView pVkCreateImageView;
				PFN_vkDestroyImageView pVkDestroyImageView;
				PFN_vkCreateFramebuffer pVkCreateFramebuffer;
				PFN_vkDestroyFramebuffer pVkDestroyFramebuffer;
				PFN_vkCreateRenderPass pVkCreateRenderPass;
				PFN_vkDestroyRenderPass pVkDestroyRenderPass;
				PFN_vkResetCommandBuffer pVkResetCommandBuffer;
				PFN_vkBeginCommandBuffer pVkBeginCommandBuffer;
				PFN_vkCmdBeginRenderPass pVkCmdBeginRenderPass;
				PFN_vkCreateShaderModule pVkCreateShaderModule;
				PFN_vkDestroyShaderModule pVkDestroyShaderModule;
				PFN_vkCreatePipelineLayout pVkCreatePipelineLayout;
				PFN_vkDestroyPipelineLayout pVkDestroyPipelineLayout;
				PFN_vkCreateDescriptorSetLayout pVkCreateDescriptorSetLayout;
				PFN_vkDestroyDescriptorSetLayout pVkDestroyDescriptorSetLayout;
				PFN_vkCreateGraphicsPipelines pVkCreateGraphicsPipelines;
				PFN_vkDestroyPipeline pVkDestroyPipeline;
				PFN_vkCmdBindPipeline pVkCmdBindPipeline;
				PFN_vkDestroyBuffer pVkDestroyBuffer;
				PFN_vkFreeMemory pVkFreeMemory;
				PFN_vkCreateBuffer pVkCreateBuffer;
				PFN_vkGetBufferMemoryRequirements pVkGetBufferMemoryRequirements;
				PFN_vkAllocateMemory pVkAllocateMemory;
				PFN_vkBindBufferMemory pVkBindBufferMemory;
				PFN_vkMapMemory pVkMapMemory;
				PFN_vkUnmapMemory pVkUnmapMemory;
				PFN_vkFlushMappedMemoryRanges pVkFlushMappedMemoryRanges;
				PFN_vkCmdBindVertexBuffers pVkCmdBindVertexBuffers;
				PFN_vkCmdBindIndexBuffer pVkCmdBindIndexBuffer;
				PFN_vkCmdSetViewport pVkCmdSetViewport;
				PFN_vkCmdSetScissor pVkCmdSetScissor;
			}Functions;
			
			typedef struct ImageData {
				VkCommandPool hCommandPool;
				VkCommandBuffer hCommandBuffer;
				VkImageView hImageView;
				VkFramebuffer hFrameBuffer;
			}ImageData;

			typedef struct BufferData {
				VkBuffer hVertexBuffer;
				VkBuffer hIndexBuffer;
				VkDeviceMemory hVertexMemory;
				VkDeviceMemory hIndexMemory;
				VkDeviceSize vertexBufferSize;
				VkDeviceSize indexBufferSize;
				VkDeviceSize alignment;
				Vertex* pLocalVertexBuffer;
				uint32_t* pLocalIndexBuffer;
				uint32_t vertexCount;
				uint32_t curOffset;
			}VertexBufferData;

			union {
				Functions _f;
				void* _fPtrs[sizeof(Functions) / sizeof(void*)];
			};

			VkPhysicalDevice _hPhysicalDevice;
			uint32_t _queueFamily;
			uint32_t _memoryTypeIndex;
			VkDevice _hDevice;
			VkRenderPass _hRenderPass;
			VkShaderModule _hShaderModuleVert;
			VkShaderModule _hShaderModuleFrag;
			VkDescriptorSetLayout _hDescriptorSetLayout;
			VkPipelineLayout _hPipelineLayout;
			VkPipeline _hPipeline;
			VkDeviceSize _bufferAlignment;
			VkCommandBuffer _hCurCommandBuffer;

			ImageData* _pImageData;
			uint32_t _imageCount;

			BufferData _triangleListBufferData;

			bool _isInit;

		public:
			Draw();

			~Draw();

			// Initializes drawing within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void beginDraw(Engine* pEngine) override;

			// Ends drawing within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void endDraw(const Engine* pEngine) override;

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. For there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in]
			// Color of the triangle list.
			void drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) override;

			// Draws text to the screen. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to a dx::Font object.
			//
			// [in] origin:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn. See length limitations implementations.
			//
			// [in] color:
			// Color of the text.
			void drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) override;

		private:
			bool getInstanceProcAddresses(HMODULE hVulkan, VkInstance hInstance);
			bool createRenderPass();
			bool createImageData(VkSwapchainKHR hSwapchain);
			bool createPipeline();
			bool createShaderModule(VkShaderModule* pShaderModule, const uint32_t shader[], size_t size);
			bool createPipelineLayout();
			bool createDescriptorSetLayout();
			bool createBufferData(BufferData* pBufferData, size_t vertexCount);
			bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, VkDeviceSize *pSize, VkBufferUsageFlagBits usage);
			uint32_t getMemoryTypeIndex(uint32_t typeBits) const;
			void destroyImageData();
		};

	}

}

