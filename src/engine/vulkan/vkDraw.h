#pragma once
#define VK_NO_PROTOTYPES
#include "include\vulkan.h"
#include "..\IDraw.h"

namespace hax {
	
	namespace vk {

		typedef struct VulkanInitData {
			PFN_vkQueuePresentKHR pVkQueuePresentKHR;
			VkDevice hDevice;
		}VulkanInitData;

		bool getVulkanInitData(VulkanInitData* initData);


		typedef struct ImageData{
			VkCommandPool hCommandPool;
			VkCommandBuffer hCommandBuffer;
			VkImageView hImageView;
			VkFramebuffer hFrameBuffer;
		}ImageData;

		class Draw : public IDraw {
		private:
			PFN_vkGetSwapchainImagesKHR _pVkGetSwapchainImagesKHR;
			PFN_vkCreateCommandPool _pVkCreateCommandPool;
			PFN_vkDestroyCommandPool _pVkDestroyCommandPool;
			PFN_vkAllocateCommandBuffers _pVkAllocateCommandBuffers;
			PFN_vkFreeCommandBuffers _pVkFreeCommandBuffers;
			PFN_vkCreateImageView _pVkCreateImageView;
			PFN_vkDestroyImageView _pVkDestroyImageView;
			PFN_vkCreateFramebuffer _pVkCreateFramebuffer;
			PFN_vkDestroyFramebuffer _pVkDestroyFramebuffer;
			PFN_vkCreateRenderPass _pVkCreateRenderPass;
			PFN_vkDestroyRenderPass _pVkDestroyRenderPass;
			PFN_vkResetCommandBuffer _pVkResetCommandBuffer;
			PFN_vkBeginCommandBuffer _pVkBeginCommandBuffer;
			PFN_vkCmdBeginRenderPass _pVkCmdBeginRenderPass;

			VkPhysicalDevice _hPhysicalDevice;
			uint32_t _queueFamily;
			VkDevice _hDevice;
			VkRenderPass _hRenderPass;

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
			bool getProcAddresses(HMODULE hVulkan, VkInstance hInstance);
			bool createRenderPass();
			bool createImageData(VkSwapchainKHR hSwapchain);
			void destroyImageData();
		};

	}

}

