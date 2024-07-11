#pragma once
#include "..\Vertex.h"
#include "..\IBackend.h"

#define VK_NO_PROTOTYPES
#include "include\vulkan.h"

namespace hax {
	
	namespace vk {

		typedef struct VulkanInitData {
			PFN_vkQueuePresentKHR pVkQueuePresentKHR;
			VkDevice hDevice;
		}VulkanInitData;

		bool getVulkanInitData(VulkanInitData* initData);

		class Backend : public IBackend {
		private:
			typedef struct Functions {
				PFN_vkGetPhysicalDeviceMemoryProperties pVkGetPhysicalDeviceMemoryProperties;
				PFN_vkGetSwapchainImagesKHR pVkGetSwapchainImagesKHR;
				PFN_vkCreateCommandPool pVkCreateCommandPool;
				PFN_vkDestroyCommandPool pVkDestroyCommandPool;
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
				PFN_vkCreateBuffer pVkCreateBuffer;
				PFN_vkDestroyBuffer pVkDestroyBuffer;
				PFN_vkGetBufferMemoryRequirements pVkGetBufferMemoryRequirements;
				PFN_vkAllocateMemory pVkAllocateMemory;
				PFN_vkBindBufferMemory pVkBindBufferMemory;
				PFN_vkFreeMemory pVkFreeMemory;
				PFN_vkAllocateCommandBuffers pVkAllocateCommandBuffers;
				PFN_vkFreeCommandBuffers pVkFreeCommandBuffers;
				PFN_vkCreateImageView pVkCreateImageView;
				PFN_vkDestroyImageView pVkDestroyImageView;
				PFN_vkCreateFramebuffer pVkCreateFramebuffer;
				PFN_vkDestroyFramebuffer pVkDestroyFramebuffer;
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
				PFN_vkCmdBindPipeline pVkCmdBindPipeline;
				PFN_vkCmdBindVertexBuffers pVkCmdBindVertexBuffers;
				PFN_vkCmdBindIndexBuffer pVkCmdBindIndexBuffer;
				PFN_vkCmdPushConstants pVkCmdPushConstants;
				PFN_vkCmdSetViewport pVkCmdSetViewport;
				PFN_vkCmdDrawIndexed pVkCmdDrawIndexed;
				PFN_vkCmdSetScissor pVkCmdSetScissor;
				PFN_vkGetDeviceQueue pVkGetDeviceQueue;
				PFN_vkQueueSubmit pVkQueueSubmit;
			}Functions;
			
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

			typedef struct ImageData {
				VkCommandBuffer hCommandBuffer;
				VkImageView hImageView;
				VkFramebuffer hFrameBuffer;
				BufferData triangleListBufferData;
				BufferData pointListBufferData;
				VkFence hFence;
			}ImageData;

			VkQueue _hQueue;
			const VkPresentInfoKHR* _phPresentInfo;
			VkDevice _hDevice;

			HMODULE _hVulkan;
			HWND _hMainWindow;
			VkInstance _hInstance;

			union {
				Functions _f;
				void* _fPtrs[sizeof(Functions) / sizeof(void*)];
			};

			VkPhysicalDevice _hPhysicalDevice;
			uint32_t _graphicsQueueFamilyIndex;
			VkRenderPass _hRenderPass;
			VkCommandPool _hCommandPool;
			VkShaderModule _hShaderModuleVert;
			VkShaderModule _hShaderModuleFrag;
			VkDescriptorSetLayout _hDescriptorSetLayout;
			VkPipelineLayout _hPipelineLayout;
			VkPipeline _hTriangleListPipeline;
			VkPipeline _hPointListPipeline;
			VkPhysicalDeviceMemoryProperties _memoryProperties;
			VkQueue _hFirstGraphicsQueue;

			ImageData* _pImageDataArray;
			uint32_t _imageCount;
			VkDeviceSize _bufferAlignment;
			ImageData* _pCurImageData;
			RECT _viewport;

			bool _isInit;
			bool _isBegin;

		public:
			Backend();

			~Backend();

			// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pArg1:
			// Pass the VkQueue.
			//
			// [in] pArg2:
			// Pass the VkPresentInfoKHR*.
			//
			// [in] pArg3:
			// Pass the device handle that was retrieved by vk::getVulkanInitData().
			virtual void beginFrame(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr) override;

			// Ends the current frame within a hook. Should be called by an Engine object.
			virtual void endFrame() override;

			// Gets the resolution of the current frame. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [out] frameWidth:
			// Pointer that receives the current frame width in pixel.
			//
			// [out] frameHeight:
			// Pointer that receives the current frame height in pixel.
			virtual void getFrameResolution(float* frameWidth, float* frameHeight) override;

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. From there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in] color:
			// Color of each triangle.
			void drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) override;

			// Draws a point list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] coordinate:
			// Screen coordinates of the points in the list.
			// 
			// [in] count:
			// Count of the points in the list.
			// 
			// [in] color:
			// Color of each point.
			//
			// [in] offest:
			// Offset by which each point is drawn.
			virtual void drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) override;

		private:
			bool initialize();
			bool getProcAddresses();
			bool createRenderPass();
			bool createCommandPool();
			VkPipeline createPipeline(VkPrimitiveTopology topology);
			VkShaderModule createShaderModule(const BYTE shader[], size_t size) const;
			bool createPipelineLayout();
			bool createDescriptorSetLayout();
			bool resizeImageDataArray(VkSwapchainKHR hSwapchain, uint32_t imageCount);
			void destroyImageDataArray();
			void destroyImageData(ImageData* pImageData) const;
			bool createFramebuffers(VkSwapchainKHR hSwapchain);
			void destroyFramebuffers();
			bool createBufferData(BufferData* pBufferData, size_t vertexCount);
			void destroyBufferData(BufferData* pBufferData) const;
			bool createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, VkDeviceSize* pSize, VkBufferUsageFlagBits usage);
			uint32_t getMemoryTypeIndex(uint32_t typeBits) const;
			bool mapBufferData(BufferData* pBufferData) const;
			bool beginCommandBuffer(VkCommandBuffer hCommandBuffer) const;
			void beginRenderPass(VkCommandBuffer hCommandBuffer, VkFramebuffer hFramebuffer) const;
			void copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f });
			bool resizeBufferData(BufferData* pBufferData, size_t newVertexCount);
			void drawBufferData(BufferData* pBufferData, VkCommandBuffer hCommandBuffer) const;
		};

	}

}

