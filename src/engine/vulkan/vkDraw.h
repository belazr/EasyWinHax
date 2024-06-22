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
				PFN_vkCreateFence pVkCreateFence;
				PFN_vkDestroyFence pVkDestroyFence;
				PFN_vkWaitForFences pVkWaitForFences;
				PFN_vkResetFences pVkResetFences;
				PFN_vkResetCommandBuffer pVkResetCommandBuffer;
				PFN_vkBeginCommandBuffer pVkBeginCommandBuffer;
				PFN_vkEndCommandBuffer pVkEndCommandBuffer;
				PFN_vkCmdBeginRenderPass pVkCmdBeginRenderPass;
				PFN_vkCmdEndRenderPass pVkCmdEndRenderPass;
				PFN_vkMapMemory pVkMapMemory;
				PFN_vkUnmapMemory pVkUnmapMemory;
				PFN_vkFlushMappedMemoryRanges pVkFlushMappedMemoryRanges;
				PFN_vkCmdBindVertexBuffers pVkCmdBindVertexBuffers;
				PFN_vkCmdBindIndexBuffer pVkCmdBindIndexBuffer;
				PFN_vkCmdPushConstants pVkCmdPushConstants;
				PFN_vkCmdSetViewport pVkCmdSetViewport;
				PFN_vkCmdDrawIndexed pVkCmdDrawIndexed;
				PFN_vkCmdSetScissor pVkCmdSetScissor;
				PFN_vkGetDeviceQueue pVkGetDeviceQueue;
				PFN_vkQueueSubmit pVkQueueSubmit;
			}Functions;
			
			typedef struct ImageData {
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
				Vertex* pLocalVertexBuffer;
				uint32_t* pLocalIndexBuffer;
				uint32_t curOffset;
			}BufferData;

			union {
				Functions _f;
				void* _fPtrs[sizeof(Functions) / sizeof(void*)];
			};

			HMODULE _hVulkan;

			VkInstance _hInstance;
			VkPhysicalDevice _hPhysicalDevice;
			uint32_t _graphicsQueueFamilyIndex;
			VkDevice _hDevice;
			VkRenderPass _hRenderPass;
			VkShaderModule _hShaderModuleVert;
			VkShaderModule _hShaderModuleFrag;
			VkDescriptorSetLayout _hDescriptorSetLayout;
			VkPipelineLayout _hPipelineLayout;
			VkPipeline _hPipeline;
			VkCommandPool _hCommandPool;
			VkCommandBuffer _hCommandBuffer;
			VkPhysicalDeviceMemoryProperties _memoryProperties;
			VkFence _hFence;
			VkDeviceSize _bufferAlignment;

			BufferData _triangleListBufferData;
			ImageData* _pImageData;
			uint32_t _imageCount;

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
			bool getProcAddresses();
			bool createRenderPass();
			bool createImageData(VkSwapchainKHR hSwapchain);
			void destroyImageData();
			bool createPipeline();
			bool createShaderModule(VkShaderModule* pShaderModule, const BYTE shader[], size_t size);
			bool createPipelineLayout();
			bool createDescriptorSetLayout();
			bool createCommandBuffer();
			bool createBufferData(BufferData* pBufferData, size_t vertexCount);
			void destroyBufferData(BufferData* pBufferData) const;
			bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, VkDeviceSize *pSize, VkBufferUsageFlagBits usage);
			uint32_t getMemoryTypeIndex(uint32_t typeBits) const;
			void copyToBufferData(BufferData* pBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset = { 0.f, 0.f });
			bool resizeBufferData(BufferData* pBufferData, size_t newSize);
			void drawBufferData(BufferData* pBufferData) const;
		};

	}

}

