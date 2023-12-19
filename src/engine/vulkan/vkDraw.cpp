#include "vkDraw.h"
#include "..\Engine.h"
#include "..\..\proc.h"

namespace hax {

	namespace vk {

		Draw::Draw() : _hVulkan{}, _pVkDestroyInstance{}, _pVkEnumeratePhysicalDevices{}, _pVkGetPhysicalDeviceProperties{},
			_pVkGetPhysicalDeviceQueueFamilyProperties{}, _pVkCreateDevice{}, _pVkDestroyDevice{}, _pVkGetSwapchainImagesKHR{},
			_pVkCreateCommandPool{}, _pVkDestroyCommandPool{}, _pVkAllocateCommandBuffers{}, _pVkFreeCommandBuffers{}, _pVkCreateRenderPass {},
			_pAllocator{}, _hInstance{}, _hPhysicalDevice{}, _queueFamily{}, _hDevice{},
			_pImageData{},
			_isInit{} {}


		Draw::~Draw() {

			if (this->_pCommandBuffers) {
				delete[] this->_pCommandBuffers;
			}

			if (this->_pCommandPools) {
				delete[] this->_pCommandPools;
			}

			if (this->_pImages) {
				delete[] this->_pImages;
			}

			if (this->_pVkDestroyDevice && this->_hDevice) {
				this->_pVkDestroyDevice(this->_hDevice, this->_pAllocator);
			}
			
			if (this->_pVkDestroyInstance && this->_hInstance) {
				this->_pVkDestroyInstance(this->_hInstance, this->_pAllocator);
			}

		}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				this->_hVulkan = proc::in::getModuleHandle("vulkan-1.dll");

				if (!this->_hVulkan) return;

				if (!this->createInstance()) return;

				if (!this->getProcAddresses()) return;

				if (!this->selectGpu()) return;

				if (!this->getQueueFamily()) return;

				if (!this->createDevice()) return;

				this->_isInit = true;
			}
			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);

			for (uint32_t i = 0u; i < pPresentInfo->swapchainCount; i++) {
				
				if (!this->createCommandPoolAndBuffers(pPresentInfo->pSwapchains[i])) return;

				VkRenderPass renderPass{};

				if (!this->createRenderPass(&renderPass)) return;

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


		bool Draw::createInstance() {
			PFN_vkCreateInstance pVkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(proc::in::getProcAddress(this->_hVulkan, "vkCreateInstance"));

			if (!pVkCreateInstance) return false;

			constexpr const char* EXTENSION = "VK_KHR_surface";

			VkInstanceCreateInfo createInfo {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.enabledExtensionCount = 1u;
			createInfo.ppEnabledExtensionNames = &EXTENSION;

			return pVkCreateInstance(&createInfo, this->_pAllocator, &this->_hInstance) == VkResult::VK_SUCCESS;
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
			this->_pVkCreateRenderPass = reinterpret_cast<PFN_vkCreateRenderPass>(pVkGetInstanceProcAddr(this->_hInstance, "vkCreateRenderPass"));

			if (
				!this->_pVkDestroyInstance || !this->_pVkEnumeratePhysicalDevices || !this->_pVkGetPhysicalDeviceProperties ||
				!this->_pVkGetPhysicalDeviceQueueFamilyProperties || !this->_pVkCreateDevice || !this->_pVkDestroyDevice ||
				!this->_pVkGetSwapchainImagesKHR || !this->_pVkCreateCommandPool || !this->_pVkDestroyCommandPool ||
				!this->_pVkAllocateCommandBuffers || !this->_pVkFreeCommandBuffers || !this->_pVkCreateRenderPass
			) return false;

			return true;
		}


		bool Draw::selectGpu() {
			uint32_t gpuCount = 0u;

			if (this->_pVkEnumeratePhysicalDevices(this->_hInstance, &gpuCount, nullptr) != VkResult::VK_SUCCESS) return false;

			if (!gpuCount) return false;

			const uint32_t bufferSize = gpuCount;
			VkPhysicalDevice* const pPhysicalDevices = new VkPhysicalDevice[bufferSize]{};

			if (this->_pVkEnumeratePhysicalDevices(this->_hInstance, &gpuCount, pPhysicalDevices) != VkResult::VK_SUCCESS) return false;

			for (uint32_t i = 0u; i < bufferSize; i++) {
				VkPhysicalDeviceProperties properties{};
				this->_pVkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					this->_hPhysicalDevice = pPhysicalDevices[i];
					break;
				}
			}

			delete[] pPhysicalDevices;

			return true;
		}


		bool Draw::getQueueFamily() {
			uint32_t propertiesCount = 0u;
			this->_pVkGetPhysicalDeviceQueueFamilyProperties(this->_hPhysicalDevice, &propertiesCount, nullptr);

			if (!propertiesCount) return false;

			const uint32_t bufferSize = propertiesCount;
			VkQueueFamilyProperties* const pProperties = new VkQueueFamilyProperties[bufferSize]{};

			this->_pVkGetPhysicalDeviceQueueFamilyProperties(this->_hPhysicalDevice, &propertiesCount, pProperties);

			this->_queueFamily = 0xFFFFFFFF;

			for (uint32_t i = 0u; i < bufferSize; i++) {

				if (pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					this->_queueFamily = i;
					break;
				}

			}

			delete[] pProperties;

			if (this->_queueFamily == 0xFFFFFFFF) return false;

			return true;
		}


		bool Draw::createDevice() {
			constexpr const char* EXTENSION = "VK_KHR_swapchain";
			constexpr float QUEUE_PRIORITY = 1.f;

			VkDeviceQueueCreateInfo queueInfo = {};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = this->_queueFamily;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &QUEUE_PRIORITY;

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = 1u;
			createInfo.pQueueCreateInfos = &queueInfo;
			createInfo.enabledExtensionCount = 1u;
			createInfo.ppEnabledExtensionNames = &EXTENSION;

			return this->_pVkCreateDevice(this->_hPhysicalDevice, &createInfo, this->_pAllocator, &this->_hDevice) == VkResult::VK_SUCCESS;
		}


		bool Draw::createCommandPoolAndBuffers(VkSwapchainKHR hSwapchain) {
			uint32_t imageCount = 0u;

			if (this->_pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, nullptr) != VkResult::VK_SUCCESS) return false;

			if (!imageCount) return false;

			if (imageCount != this->_imageCount) {
				freeImageData();

				this->_imageCount = imageCount;
			}

			if (!this->_pImageData) {
				this->_pImageData = new ImageData[this->_imageCount]{};
			}

			VkImage* pImages = new VkImage[this->_imageCount]{};

			if (this->_pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, this->_pImages) != VkResult::VK_SUCCESS) {
				delete[] pImages;

				return false;
			}

			for (uint32_t i = 0; i < this->_imageCount; ++i) {
				this->_pImageData[i].hImage = pImages[i];

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
			}

			return true;
		}


		void Draw::freeImageData() {

			if (this->_pImageData) {

				for (uint32_t i = 0u; i < this->_imageCount; i++) {

					if (this->_pImageData[i].hCommandBuffer) {
						this->_pVkDestroyCommandPool(this->_hDevice, this->_pImageData[i].hCommandPool, 1, this->_pImageData[i].hCommandBuffer);
					}
					
					if (this->_pImageData[i].hCommandPool) {
						this->_pVkDestroyCommandPool(this->_hDevice, this->_pImageData[i].hCommandPool, this->_pAllocator);
					}

				}

				delete[] this->_pImageData;
				this->_pImageData = nullptr;
			}

			return;
		}


		bool Draw::createRenderPass(VkRenderPass* pRenderPass) const {
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

			return this->_pVkCreateRenderPass(this->_hDevice, &info, this->_pAllocator, pRenderPass) == VkResult::VK_SUCCESS;
		}

	}

}