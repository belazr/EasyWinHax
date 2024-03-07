#include "vkDraw.h"
#include "..\Engine.h"
#include "..\..\proc.h"
#include "..\..\hooks\TrampHook.h"

namespace hax {

	namespace vk {

		static hax::in::TrampHook* pAcquireHook;
		static HANDLE hSemaphore;
		static VkDevice hDevice;

		VkResult VKAPI_CALL hkvkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) {
			hDevice = device;

			ReleaseSemaphore(hSemaphore, 1, nullptr);
			pAcquireHook->disable();
			const PFN_vkAcquireNextImageKHR pAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pAcquireHook->getOrigin());

			return pAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
		}


		static VkDevice createDummyDevice(HMODULE hVulkan);

		bool getVulkanInitData(VulkanInitData* initData) {
			const HMODULE hVulkan = hax::proc::in::getModuleHandle("vulkan-1.dll");

			if (!hVulkan) return false;

			const VkDevice hDummyDevice = createDummyDevice(hVulkan);

			if (!hDummyDevice) return false;

			PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(hVulkan, "vkGetDeviceProcAddr"));

			if (!pVkGetDeviceProcAddr) return false;

			initData->pVkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkQueuePresentKHR"));

			PFN_vkAcquireNextImageKHR pVkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkAcquireNextImageKHR"));

			hSemaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr);

			if (hSemaphore) {
				pAcquireHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(pVkAcquireNextImageKHR), reinterpret_cast<BYTE*>(hkvkAcquireNextImageKHR), 0xC);
				pAcquireHook->enable();
				WaitForSingleObject(hSemaphore, INFINITE);
				delete pAcquireHook;
			}

			initData->hDevice = hDevice;

			return true;
		}


		static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan);
		static uint32_t getQueueFamily(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		static VkDevice createDummyDevice(HMODULE hVulkan) {
			const VkPhysicalDevice hPhysicalDevice = getPhysicalDevice(hVulkan);

			if (hPhysicalDevice == VK_NULL_HANDLE) return VK_NULL_HANDLE;

			const uint32_t queueFamily = getQueueFamily(hVulkan, hPhysicalDevice);

			if (queueFamily == 0xFFFFFFFF) return VK_NULL_HANDLE;

			constexpr const char* EXTENSION = "VK_KHR_swapchain";
			constexpr float QUEUE_PRIORITY = 1.f;

			VkDeviceQueueCreateInfo queueInfo = {};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = queueFamily;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &QUEUE_PRIORITY;

			VkDeviceCreateInfo createInfo0 = {};
			createInfo0.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo0.queueCreateInfoCount = 1u;
			createInfo0.pQueueCreateInfos = &queueInfo;
			createInfo0.enabledExtensionCount = 1u;
			createInfo0.ppEnabledExtensionNames = &EXTENSION;

			PFN_vkCreateDevice pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(proc::in::getProcAddress(hVulkan, "vkCreateDevice"));

			if (!pVkCreateDevice) return VK_NULL_HANDLE;

			VkDevice hDummyDevice = VK_NULL_HANDLE;

			if (pVkCreateDevice(hPhysicalDevice, &createInfo0, nullptr, &hDummyDevice) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hDummyDevice;
		}


		static uint32_t getQueueFamily(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
			const PFN_vkGetPhysicalDeviceQueueFamilyProperties pVkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(proc::in::getProcAddress(hVulkan, "vkGetPhysicalDeviceQueueFamilyProperties"));
			
			uint32_t propertiesCount = 0u;
			pVkGetPhysicalDeviceQueueFamilyProperties(hPhysicalDevice, &propertiesCount, nullptr);

			if (!propertiesCount) return false;

			const uint32_t bufferSize = propertiesCount;
			VkQueueFamilyProperties* const pProperties = new VkQueueFamilyProperties[bufferSize]{};

			pVkGetPhysicalDeviceQueueFamilyProperties(hPhysicalDevice, &propertiesCount, pProperties);

			uint32_t queueFamily = 0xFFFFFFFF;

			for (uint32_t i = 0u; i < bufferSize; i++) {

				if (pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					queueFamily = i;
					break;
				}

			}

			delete[] pProperties;

			return queueFamily;
		}

		static VkInstance createInstance(HMODULE hVulkan);

		static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan) {
			const VkInstance hInstance = createInstance(hVulkan);
			const PFN_vkEnumeratePhysicalDevices pVkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(hax::proc::in::getProcAddress(hVulkan, "vkEnumeratePhysicalDevices"));
			const PFN_vkGetPhysicalDeviceProperties pVkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(hax::proc::in::getProcAddress(hVulkan, "vkGetPhysicalDeviceProperties"));
			
			uint32_t gpuCount = 0u;

			if (pVkEnumeratePhysicalDevices(hInstance, &gpuCount, nullptr) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			if (!gpuCount) return VK_NULL_HANDLE;

			const uint32_t bufferSize = gpuCount;
			VkPhysicalDevice* const pPhysicalDevices = new VkPhysicalDevice[bufferSize]{};

			if (pVkEnumeratePhysicalDevices(hInstance, &gpuCount, pPhysicalDevices) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			VkPhysicalDevice hPhysicalDevice = VK_NULL_HANDLE;
			
			for (uint32_t i = 0u; i < bufferSize; i++) {
				VkPhysicalDeviceProperties properties{};
				pVkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					hPhysicalDevice = pPhysicalDevices[i];
					break;
				}
			}

			delete[] pPhysicalDevices;

			return hPhysicalDevice;
		}


		static VkInstance createInstance(HMODULE hVulkan) {
			PFN_vkCreateInstance pVkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(proc::in::getProcAddress(hVulkan, "vkCreateInstance"));

			if (!pVkCreateInstance) return VK_NULL_HANDLE;

			constexpr const char* EXTENSION = "VK_KHR_surface";

			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.enabledExtensionCount = 1u;
			createInfo.ppEnabledExtensionNames = &EXTENSION;

			VkInstance hInstance = VK_NULL_HANDLE;

			if (pVkCreateInstance(&createInfo, nullptr, &hInstance) != VkResult::VK_SUCCESS) {
				return VK_NULL_HANDLE;
			}

			return hInstance;
		}


		Draw::Draw() : _hVulkan{}, _pVkDestroyInstance{}, _pVkEnumeratePhysicalDevices{}, _pVkGetPhysicalDeviceProperties{},
			_pVkGetPhysicalDeviceQueueFamilyProperties{}, _pVkCreateDevice{}, _pVkDestroyDevice{}, _pVkGetSwapchainImagesKHR{},
			_pVkCreateCommandPool{}, _pVkDestroyCommandPool{}, _pVkAllocateCommandBuffers{}, _pVkFreeCommandBuffers{},
			_pVkCreateImageView{}, _pVkDestroyImageView{}, _pVkCreateFramebuffer {}, _pVkDestroyFramebuffer{},
			_pVkCreateRenderPass{}, _pVkDestroyRenderPass{},
			_pAllocator{}, _hInstance{}, _hPhysicalDevice{}, _queueFamily{}, _hDevice {}, _hRenderPass{},
			_pImageData{}, _imageCount{},
			_isInit{} {}


		Draw::~Draw() {
			destroyImageData();

			if (this->_pVkDestroyRenderPass && this->_hDevice && this->_hRenderPass) {
				this->_pVkDestroyRenderPass(this->_hDevice, this->_hRenderPass, this->_pAllocator);
			}
			
			if (this->_pVkDestroyInstance && this->_hInstance) {
				this->_pVkDestroyInstance(this->_hInstance, this->_pAllocator);
			}

		}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {

				if (!this->getProcAddresses()) return;

				this->_isInit = true;
			}

			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);
			this->_hDevice = reinterpret_cast<VkDevice>(pEngine->pHookArg3);

			if (!pPresentInfo || !this->_hDevice) return;

			for (uint32_t i = 0u; i < pPresentInfo->swapchainCount; i++) {
				
				if (!this->createRenderPass()) return;

				if (!this->createImageData(pPresentInfo->pSwapchains[i])) return;

			}

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			
			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {


			return;
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			return;
		}


		bool Draw::getProcAddresses() {
			PFN_vkGetInstanceProcAddr pVkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetInstanceProcAddr"));

			if (!pVkGetInstanceProcAddr) return false;

			this->_pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyInstance"));
			this->_pVkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(pVkGetInstanceProcAddr(this->_hInstance, "vkEnumeratePhysicalDevices"));
			this->_pVkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(pVkGetInstanceProcAddr(this->_hInstance, "vkGetPhysicalDeviceProperties"));
			this->_pVkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(pVkGetInstanceProcAddr(this->_hInstance, "vkGetPhysicalDeviceQueueFamilyProperties"));
			this->_pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateDevice"));
			this->_pVkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyDevice"));
			this->_pVkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(pVkGetInstanceProcAddr(this->_hInstance, "vkGetSwapchainImagesKHR"));
			this->_pVkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateCommandPool"));
			this->_pVkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyCommandPool"));
			this->_pVkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(pVkGetInstanceProcAddr(this->_hInstance, "vkAllocateCommandBuffers"));
			this->_pVkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(pVkGetInstanceProcAddr(this->_hInstance, "vkFreeCommandBuffers"));
			this->_pVkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateImageView"));
			this->_pVkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyImageView"));
			this->_pVkCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateFramebuffer"));
			this->_pVkDestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyFramebuffer"));
			this->_pVkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateRenderPass"));
			this->_pVkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(pVkGetInstanceProcAddr(this->_hInstance, "vkDestroyRenderPass"));

			if (
				!this->_pVkDestroyInstance || !this->_pVkEnumeratePhysicalDevices || !this->_pVkGetPhysicalDeviceProperties ||
				!this->_pVkGetPhysicalDeviceQueueFamilyProperties || !this->_pVkCreateDevice || !this->_pVkDestroyDevice ||
				!this->_pVkGetSwapchainImagesKHR || !this->_pVkCreateCommandPool || !this->_pVkDestroyCommandPool ||
				!this->_pVkAllocateCommandBuffers || !this->_pVkFreeCommandBuffers || !this->_pVkCreateRenderPass ||
				!this->_pVkDestroyRenderPass || !this->_pVkCreateImageView || !this->_pVkDestroyImageView ||
				!this->_pVkCreateFramebuffer || !this->_pVkDestroyFramebuffer
			) return false;

			return true;
		}


		bool Draw::createRenderPass() {
			VkAttachmentDescription attachment{};
			attachment.format = VK_FORMAT_B8G8R8A8_UNORM;
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachment{};
			colorAttachment.attachment = 0u;
			colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1u;
			subpass.pColorAttachments = &colorAttachment;

			VkRenderPassCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			info.attachmentCount = 1u;
			info.pAttachments = &attachment;
			info.subpassCount = 1u;
			info.pSubpasses = &subpass;

			return this->_pVkCreateRenderPass(this->_hDevice, &info, this->_pAllocator, &this->_hRenderPass) == VkResult::VK_SUCCESS;
		}


		bool Draw::createImageData(VkSwapchainKHR hSwapchain) {
			uint32_t imageCount = 0u;

			if (this->_pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, nullptr) != VkResult::VK_SUCCESS) return false;

			if (!imageCount) return false;

			if (imageCount != this->_imageCount) {
				destroyImageData();

				this->_imageCount = imageCount;
			}

			VkImage* pImages = new VkImage[this->_imageCount]{};

			if (this->_pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, pImages) != VkResult::VK_SUCCESS) {
				delete[] pImages;

				return false;
			}

			VkImageSubresourceRange imageRange{};
			imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageRange.baseMipLevel = 0u;
			imageRange.levelCount = 1u;
			imageRange.baseArrayLayer = 0u;
			imageRange.layerCount = 1u;

			VkImageViewCreateInfo imageViewInfo{};
			imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
			imageViewInfo.subresourceRange = imageRange;

			VkFramebufferCreateInfo frameBufferInfo = {};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.renderPass = this->_hRenderPass;
			frameBufferInfo.attachmentCount = 1;
			frameBufferInfo.layers = 1;

			for (uint32_t i = 0; i < this->_imageCount; ++i) {
				VkCommandPoolCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				createInfo.queueFamilyIndex = this->_queueFamily;

				this->_pVkCreateCommandPool(this->_hDevice, &createInfo, this->_pAllocator, &this->_pImageData[i].hCommandPool);

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = this->_pImageData[i].hCommandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = 1;

				this->_pVkAllocateCommandBuffers(this->_hDevice, &allocInfo, &this->_pImageData[i].hCommandBuffer);

				imageViewInfo.image = pImages[i];
				
				if (this->_pVkCreateImageView(this->_hDevice, &imageViewInfo, this->_pAllocator, &this->_pImageData[i].hImageView) != VkResult::VK_SUCCESS) continue;

				frameBufferInfo.pAttachments = &this->_pImageData[i].hImageView;
				this->_pVkCreateFramebuffer(this->_hDevice, &frameBufferInfo, this->_pAllocator, &this->_pImageData[i].hFrameBuffer);
			}

			delete[] pImages;

			return true;
		}


		void Draw::destroyImageData() {

			if (!this->_pVkFreeCommandBuffers || !this->_pVkDestroyCommandPool) return;

			if (this->_pImageData) {

				for (uint32_t i = 0u; i < this->_imageCount; i++) {

					if (this->_pImageData[i].hFrameBuffer != VK_NULL_HANDLE) {
						this->_pVkDestroyFramebuffer(this->_hDevice, this->_pImageData[i].hFrameBuffer, this->_pAllocator);
					}
					
					if (this->_pImageData[i].hImageView != VK_NULL_HANDLE) {
						this->_pVkDestroyImageView(this->_hDevice, this->_pImageData[i].hImageView, this->_pAllocator);
					}

					if (this->_pImageData[i].hCommandBuffer != VK_NULL_HANDLE) {
						this->_pVkFreeCommandBuffers(this->_hDevice, this->_pImageData[i].hCommandPool, 1, &this->_pImageData[i].hCommandBuffer);
					}
					
					if (this->_pImageData[i].hCommandPool != VK_NULL_HANDLE) {
						this->_pVkDestroyCommandPool(this->_hDevice, this->_pImageData[i].hCommandPool, this->_pAllocator);
					}

				}

				delete[] this->_pImageData;
				this->_pImageData = nullptr;
			}

			return;
		}

	}

}