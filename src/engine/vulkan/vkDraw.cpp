#include "vkDraw.h"
#include "..\Engine.h"
#include "..\..\proc.h"
#include "..\..\hooks\TrampHook.h"

namespace hax {

	namespace vk {

		static hax::in::TrampHook* pAcquireHook;
		static HANDLE hSemaphore;
		static VkDevice hDevice;

		static VkResult VKAPI_CALL hkvkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) {
			hDevice = device;
			pAcquireHook->disable();
			const PFN_vkAcquireNextImageKHR pAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pAcquireHook->getOrigin());
			ReleaseSemaphore(hSemaphore, 1, nullptr);

			return pAcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);
		}


		static VkInstance createInstance(HMODULE hVulkan);
		static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan, VkInstance hInstance);
		static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		bool getVulkanInitData(VulkanInitData* initData) {
			const HMODULE hVulkan = hax::proc::in::getModuleHandle("vulkan-1.dll");

			if (!hVulkan) return false;

			const PFN_vkDestroyDevice pVkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(proc::in::getProcAddress(hVulkan, "vkDestroyDevice"));
			const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(hVulkan, "vkDestroyInstance"));

			if (!pVkDestroyDevice || !pVkDestroyInstance) return false;

			const VkInstance hInstance = createInstance(hVulkan);

			if (hInstance == VK_NULL_HANDLE) return false;

			VkPhysicalDevice hPhysicalDevice = getPhysicalDevice(hVulkan, hInstance);
			//pVkDestroyInstance(hInstance, nullptr);

			if (hPhysicalDevice == VK_NULL_HANDLE) return false;

			const VkDevice hDummyDevice = createDummyDevice(hVulkan, hPhysicalDevice);

			if (hDummyDevice == VK_NULL_HANDLE) return false;

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
				initData->hDevice = hDevice;
			}

			return true;
		}


		static VkInstance createInstance(HMODULE hVulkan) {
			const PFN_vkCreateInstance pVkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(proc::in::getProcAddress(hVulkan, "vkCreateInstance"));

			if (!pVkCreateInstance) return VK_NULL_HANDLE;

			constexpr const char* EXTENSION = "VK_KHR_surface";

			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.enabledExtensionCount = 1u;
			createInfo.ppEnabledExtensionNames = &EXTENSION;

			VkInstance hInstance = VK_NULL_HANDLE;

			if (pVkCreateInstance(&createInfo, nullptr, &hInstance) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hInstance;
		}


		static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan, VkInstance hInstance) {
			const PFN_vkEnumeratePhysicalDevices pVkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(hax::proc::in::getProcAddress(hVulkan, "vkEnumeratePhysicalDevices"));
			const PFN_vkGetPhysicalDeviceProperties pVkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(hax::proc::in::getProcAddress(hVulkan, "vkGetPhysicalDeviceProperties"));
			
			if (!pVkEnumeratePhysicalDevices || !pVkGetPhysicalDeviceProperties) return VK_NULL_HANDLE;
			
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


		static uint32_t getQueueFamily(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
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

			const PFN_vkCreateDevice pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(proc::in::getProcAddress(hVulkan, "vkCreateDevice"));

			if (!pVkCreateDevice) return VK_NULL_HANDLE;

			VkDevice hDummyDevice = VK_NULL_HANDLE;

			if (pVkCreateDevice(hPhysicalDevice, &createInfo0, nullptr, &hDummyDevice) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hDummyDevice;
		}


		static uint32_t getQueueFamily(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
			const PFN_vkGetPhysicalDeviceQueueFamilyProperties pVkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(proc::in::getProcAddress(hVulkan, "vkGetPhysicalDeviceQueueFamilyProperties"));

			if (!pVkGetPhysicalDeviceQueueFamilyProperties) return 0xFFFFFFFF;
			
			uint32_t propertiesCount = 0u;
			pVkGetPhysicalDeviceQueueFamilyProperties(hPhysicalDevice, &propertiesCount, nullptr);

			if (!propertiesCount) return 0xFFFFFFFF;

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


		Draw::Draw() : _pVkGetSwapchainImagesKHR{}, _pVkCreateCommandPool{}, _pVkDestroyCommandPool{},
			_pVkAllocateCommandBuffers{}, _pVkFreeCommandBuffers{}, _pVkCreateImageView{}, _pVkDestroyImageView{},
			_pVkCreateFramebuffer{}, _pVkDestroyFramebuffer{}, _pVkCreateRenderPass{}, _pVkDestroyRenderPass{},
			_pVkResetCommandBuffer{}, _pVkBeginCommandBuffer{}, _pVkCmdBeginRenderPass{},
			_hPhysicalDevice{}, _queueFamily{}, _hDevice {}, _hRenderPass{},
			_pImageData{}, _imageCount{},
			_isInit{} {}


		Draw::~Draw() {
			destroyImageData();

			if (this->_pVkDestroyRenderPass && this->_hDevice && this->_hRenderPass) {
				this->_pVkDestroyRenderPass(this->_hDevice, this->_hRenderPass, nullptr);
			}
			
		}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				const HMODULE hVulkan = proc::in::getModuleHandle("vulkan-1.dll");

				if (!hVulkan) return;

				const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(hVulkan, "vkDestroyInstance"));

				if (!pVkDestroyInstance) return;

				const VkInstance hInstance = createInstance(hVulkan);

				if (hInstance == VK_NULL_HANDLE) return;

				if (!this->getProcAddresses(hVulkan, hInstance)) {
					pVkDestroyInstance(hInstance, nullptr);

					return;
				}

				this->_hPhysicalDevice = getPhysicalDevice(hVulkan, hInstance);
				pVkDestroyInstance(hInstance, nullptr);

				if (this->_hPhysicalDevice == VK_NULL_HANDLE) return;

				this->_queueFamily = getQueueFamily(hVulkan, this->_hPhysicalDevice);

				if (this->_queueFamily == 0xFFFFFFFF) return;

				this->_isInit = true;
			}

			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);
			this->_hDevice = reinterpret_cast<VkDevice>(pEngine->pHookArg3);

			if (!pPresentInfo || !this->_hDevice) return;

			for (uint32_t i = 0u; i < pPresentInfo->swapchainCount; i++) {
				
				if (!this->createRenderPass()) return;

				if (!this->createImageData(pPresentInfo->pSwapchains[i])) return;

				const ImageData curImageData = this->_pImageData[pPresentInfo->pImageIndices[i]];

				if (curImageData.hCommandBuffer == VK_NULL_HANDLE) return;
				
				if (this->_pVkResetCommandBuffer(curImageData.hCommandBuffer, 0) != VkResult::VK_SUCCESS) return;

				VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
				cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				if (this->_pVkBeginCommandBuffer(curImageData.hCommandBuffer, &cmdBufferBeginInfo) != VkResult::VK_SUCCESS) return;

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = this->_hRenderPass;
				renderPassBeginInfo.framebuffer = curImageData.hFrameBuffer;
				renderPassBeginInfo.renderArea.extent.width = 1366;
				renderPassBeginInfo.renderArea.extent.height = 768;

				this->_pVkCmdBeginRenderPass(curImageData.hCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
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


		bool Draw::getProcAddresses(HMODULE hVulkan, VkInstance hInstance) {
			const PFN_vkGetInstanceProcAddr pVkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc::in::getProcAddress(hVulkan, "vkGetInstanceProcAddr"));
			
			if (!pVkGetInstanceProcAddr) return false;

			this->_pVkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(pVkGetInstanceProcAddr(hInstance, "vkGetSwapchainImagesKHR"));
			this->_pVkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(pVkGetInstanceProcAddr(hInstance, "vkCreateCommandPool"));
			this->_pVkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(pVkGetInstanceProcAddr(hInstance, "vkDestroyCommandPool"));
			this->_pVkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(pVkGetInstanceProcAddr(hInstance, "vkAllocateCommandBuffers"));
			this->_pVkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(pVkGetInstanceProcAddr(hInstance, "vkFreeCommandBuffers"));
			this->_pVkCreateImageView = reinterpret_cast<PFN_vkCreateImageView>(pVkGetInstanceProcAddr(hInstance, "vkCreateImageView"));
			this->_pVkDestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(pVkGetInstanceProcAddr(hInstance, "vkDestroyImageView"));
			this->_pVkCreateFramebuffer = reinterpret_cast<PFN_vkCreateFramebuffer>(pVkGetInstanceProcAddr(hInstance, "vkCreateFramebuffer"));
			this->_pVkDestroyFramebuffer = reinterpret_cast<PFN_vkDestroyFramebuffer>(pVkGetInstanceProcAddr(hInstance, "vkDestroyFramebuffer"));
			this->_pVkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(pVkGetInstanceProcAddr(hInstance, "vkCreateRenderPass"));
			this->_pVkDestroyRenderPass = reinterpret_cast<PFN_vkDestroyRenderPass>(pVkGetInstanceProcAddr(hInstance, "vkDestroyRenderPass"));
			this->_pVkResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(pVkGetInstanceProcAddr(hInstance, "vkResetCommandBuffer"));
			this->_pVkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(pVkGetInstanceProcAddr(hInstance, "vkBeginCommandBuffer"));
			this->_pVkCmdBeginRenderPass = reinterpret_cast<PFN_vkCmdBeginRenderPass>(pVkGetInstanceProcAddr(hInstance, "vkCmdBeginRenderPass"));

			if (
				!this->_pVkGetSwapchainImagesKHR || !this->_pVkCreateCommandPool || !this->_pVkDestroyCommandPool ||
				!this->_pVkAllocateCommandBuffers || !this->_pVkFreeCommandBuffers || !this->_pVkCreateRenderPass ||
				!this->_pVkDestroyRenderPass || !this->_pVkCreateImageView || !this->_pVkDestroyImageView ||
				!this->_pVkCreateFramebuffer || !this->_pVkDestroyFramebuffer || !this->_pVkResetCommandBuffer ||
				!this->_pVkBeginCommandBuffer || !this->_pVkCmdBeginRenderPass
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

			return this->_pVkCreateRenderPass(this->_hDevice, &info, nullptr, &this->_hRenderPass) == VkResult::VK_SUCCESS;
		}


		bool Draw::createImageData(VkSwapchainKHR hSwapchain) {
			uint32_t imageCount = 0u;

			if (this->_pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, nullptr) != VkResult::VK_SUCCESS) return false;

			if (!imageCount) return false;

			if (imageCount != this->_imageCount) {
				destroyImageData();

				this->_imageCount = imageCount;
			}

			if (!this->_pImageData) {
				this->_pImageData = new ImageData[this->_imageCount]{};
			}

			VkImage* const pImages = new VkImage[this->_imageCount]{};

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

				if (this->_pVkCreateCommandPool(this->_hDevice, &createInfo, nullptr, &this->_pImageData[i].hCommandPool) != VkResult::VK_SUCCESS) continue;

				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = this->_pImageData[i].hCommandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = 1;

				if (this->_pVkAllocateCommandBuffers(this->_hDevice, &allocInfo, &this->_pImageData[i].hCommandBuffer) != VkResult::VK_SUCCESS) continue;

				imageViewInfo.image = pImages[i];
				
				if (this->_pVkCreateImageView(this->_hDevice, &imageViewInfo, nullptr, &this->_pImageData[i].hImageView) != VkResult::VK_SUCCESS) continue;

				frameBufferInfo.pAttachments = &this->_pImageData[i].hImageView;
				this->_pVkCreateFramebuffer(this->_hDevice, &frameBufferInfo, nullptr, &this->_pImageData[i].hFrameBuffer);
			}

			delete[] pImages;

			return true;
		}


		void Draw::destroyImageData() {

			if (!this->_pVkFreeCommandBuffers || !this->_pVkDestroyCommandPool) return;

			if (this->_pImageData) {

				for (uint32_t i = 0u; i < this->_imageCount; i++) {

					if (this->_pImageData[i].hFrameBuffer != VK_NULL_HANDLE) {
						this->_pVkDestroyFramebuffer(this->_hDevice, this->_pImageData[i].hFrameBuffer, nullptr);
					}
					
					if (this->_pImageData[i].hImageView != VK_NULL_HANDLE) {
						this->_pVkDestroyImageView(this->_hDevice, this->_pImageData[i].hImageView, nullptr);
					}

					if (this->_pImageData[i].hCommandBuffer != VK_NULL_HANDLE) {
						this->_pVkFreeCommandBuffers(this->_hDevice, this->_pImageData[i].hCommandPool, 1, &this->_pImageData[i].hCommandBuffer);
					}
					
					if (this->_pImageData[i].hCommandPool != VK_NULL_HANDLE) {
						this->_pVkDestroyCommandPool(this->_hDevice, this->_pImageData[i].hCommandPool, nullptr);
					}

				}

				delete[] this->_pImageData;
				this->_pImageData = nullptr;
			}

			return;
		}

	}

}