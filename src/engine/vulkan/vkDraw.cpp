#include "vkDraw.h"
#include "..\Engine.h"
#include "..\Vertex.h"
#include "..\font\Font.h"
#include "..\..\proc.h"
#include "..\..\hooks\TrampHook.h"
#include "..\..\Bench.h"

namespace hax {

	namespace vk {

		static hax::in::TrampHook* pAcquireHook;
		static HANDLE hHookSemaphore;
		static VkDevice hHookDevice;

		static VkResult VKAPI_CALL hkvkAcquireNextImageKHR(VkDevice hDevice, VkSwapchainKHR hSwapchain, uint64_t timeout, VkSemaphore hSemaphore, VkFence hFence, uint32_t* pImageIndex) {
			hHookDevice = hDevice;
			pAcquireHook->disable();
			const PFN_vkAcquireNextImageKHR pAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pAcquireHook->getOrigin());
			ReleaseSemaphore(hHookSemaphore, 1, nullptr);

			return pAcquireNextImageKHR(hDevice, hSwapchain, timeout, hSemaphore, hFence, pImageIndex);
		}


		static VkInstance createInstance(HMODULE hVulkan);
		static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan, VkInstance hInstance);
		static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		bool getVulkanInitData(VulkanInitData* initData) {
			const HMODULE hVulkan = hax::proc::in::getModuleHandle("vulkan-1.dll");

			if (!hVulkan) return false;

			const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(hVulkan, "vkDestroyInstance"));
			const PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(hVulkan, "vkGetDeviceProcAddr"));
			const PFN_vkDestroyDevice pVkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(proc::in::getProcAddress(hVulkan, "vkDestroyDevice"));

			if (!pVkDestroyInstance || !pVkGetDeviceProcAddr || !pVkDestroyDevice) return false;

			const VkInstance hInstance = createInstance(hVulkan);

			if (hInstance == VK_NULL_HANDLE) return false;

			VkPhysicalDevice hPhysicalDevice = getPhysicalDevice(hVulkan, hInstance);

			if (hPhysicalDevice == VK_NULL_HANDLE) {
				pVkDestroyInstance(hInstance, nullptr);

				return false;
			}

			const VkDevice hDummyDevice = createDummyDevice(hVulkan, hPhysicalDevice);

			if (hDummyDevice == VK_NULL_HANDLE) {
				pVkDestroyInstance(hInstance, nullptr);

				return false;
			}

			initData->pVkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkQueuePresentKHR"));
			const PFN_vkAcquireNextImageKHR pVkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkAcquireNextImageKHR"));

			pVkDestroyDevice(hDummyDevice, nullptr);
			pVkDestroyInstance(hInstance, nullptr);

			if (!initData->pVkQueuePresentKHR || !pVkAcquireNextImageKHR) return false;

			hHookSemaphore = CreateSemaphoreA(nullptr, 0, 1, nullptr);

			if (!hHookSemaphore) return false;

			pAcquireHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(pVkAcquireNextImageKHR), reinterpret_cast<BYTE*>(hkvkAcquireNextImageKHR), 0xC);
			pAcquireHook->enable();
			WaitForSingleObject(hHookSemaphore, INFINITE);
			delete pAcquireHook;
			
			initData->hDevice = hHookDevice;

			return initData->hDevice != VK_NULL_HANDLE;
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

			VkPhysicalDevice* const pPhysicalDeviceArray = new VkPhysicalDevice[gpuCount]{};

			if (pVkEnumeratePhysicalDevices(hInstance, &gpuCount, pPhysicalDeviceArray) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			VkPhysicalDevice hPhysicalDevice = VK_NULL_HANDLE;

			#pragma warning(push)
			#pragma warning(disable:6385)

			for (uint32_t i = 0u; i < gpuCount; i++) {
				VkPhysicalDeviceProperties properties{};
				pVkGetPhysicalDeviceProperties(pPhysicalDeviceArray[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					hPhysicalDevice = pPhysicalDeviceArray[i];
					break;
				}
			}

			#pragma warning(pop)

			delete[] pPhysicalDeviceArray;

			return hPhysicalDevice;
		}


		static uint32_t getGraphicsQueueFamilyIndex(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
			const PFN_vkCreateDevice pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(proc::in::getProcAddress(hVulkan, "vkCreateDevice"));

			if (!pVkCreateDevice) return VK_NULL_HANDLE;
			
			const uint32_t graphicsQueueFamilyIndex = getGraphicsQueueFamilyIndex(hVulkan, hPhysicalDevice);

			if (graphicsQueueFamilyIndex == 0xFFFFFFFF) return VK_NULL_HANDLE;

			constexpr float QUEUE_PRIORITY = 1.f;

			VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
			deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
			deviceQueueCreateInfo.queueCount = 1u;
			deviceQueueCreateInfo.pQueuePriorities = &QUEUE_PRIORITY;

			constexpr const char* EXTENSION = "VK_KHR_swapchain";

			VkDeviceCreateInfo deviceCreateInfo{};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.queueCreateInfoCount = 1u;
			deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
			deviceCreateInfo.enabledExtensionCount = 1u;
			deviceCreateInfo.ppEnabledExtensionNames = &EXTENSION;

			VkDevice hDummyDevice = VK_NULL_HANDLE;

			if (pVkCreateDevice(hPhysicalDevice, &deviceCreateInfo, nullptr, &hDummyDevice) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hDummyDevice;
		}


		static uint32_t getGraphicsQueueFamilyIndex(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
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


		Draw::Draw() :
			_hVulkan{}, _hMainWindow{}, _hDevice{}, _hInstance{}, _f{},
			_hPhysicalDevice{}, _graphicsQueueFamilyIndex{ 0xFFFFFFFF }, _hRenderPass{}, _hCommandPool{},
			_hShaderModuleVert{}, _hShaderModuleFrag{}, _hDescriptorSetLayout{}, _hPipelineLayout{},
			_hTriangleListPipeline{}, _hPointListPipeline{}, _memoryProperties{}, _hFirstGraphicsQueue{},
			_pImageDataArray{}, _imageCount{}, _bufferAlignment{ 4ull }, _pCurImageData{}, _windowRect{},
			_isInit{}, _isBegin{} {}


		Draw::~Draw() {
			
			if (this->_hVulkan) {
				const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(this->_hVulkan, "vkDestroyInstance"));

				if (pVkDestroyInstance && this->_hInstance != VK_NULL_HANDLE) {
					pVkDestroyInstance(this->_hInstance, nullptr);
				}

			}

			if (this->_hDevice == VK_NULL_HANDLE) return;

			if (
				this->_f.pVkFreeCommandBuffers && this->_f.pVkDestroyFramebuffer && this->_f.pVkDestroyImageView && this->_f.pVkDestroyFence &&
				this->_f.pVkUnmapMemory && this->_f.pVkFreeMemory && this->_f.pVkDestroyBuffer
			) {
				this->destroyImageDataArray();
			}

			if (this->_f.pVkDestroyPipeline) {

				if (this->_hPointListPipeline != VK_NULL_HANDLE) {
					this->_f.pVkDestroyPipeline(this->_hDevice, this->_hPointListPipeline, nullptr);
				}

				if (this->_hTriangleListPipeline != VK_NULL_HANDLE) {
					this->_f.pVkDestroyPipeline(this->_hDevice, this->_hTriangleListPipeline, nullptr);
				}

			}

			if (this->_f.pVkDestroyPipelineLayout && this->_hPipelineLayout != VK_NULL_HANDLE) {
				this->_f.pVkDestroyPipelineLayout(this->_hDevice, this->_hPipelineLayout, nullptr);
			}

			if (this->_f.pVkDestroyDescriptorSetLayout && this->_hDescriptorSetLayout != VK_NULL_HANDLE) {
				this->_f.pVkDestroyDescriptorSetLayout(this->_hDevice, this->_hDescriptorSetLayout, nullptr);
			}

			if (this->_f.pVkDestroyShaderModule && this->_hShaderModuleFrag != VK_NULL_HANDLE) {
				this->_f.pVkDestroyShaderModule(this->_hDevice, this->_hShaderModuleFrag, nullptr);
			}

			if (this->_f.pVkDestroyShaderModule && this->_hShaderModuleVert != VK_NULL_HANDLE) {
				this->_f.pVkDestroyShaderModule(this->_hDevice, this->_hShaderModuleVert, nullptr);
			}

			if (this->_f.pVkDestroyCommandPool && this->_hCommandPool != VK_NULL_HANDLE) {
				this->_f.pVkDestroyCommandPool(this->_hDevice, this->_hCommandPool, nullptr);
			}

			if (this->_f.pVkDestroyRenderPass && this->_hRenderPass != VK_NULL_HANDLE) {
				this->_f.pVkDestroyRenderPass(this->_hDevice, this->_hRenderPass, nullptr);
			}

		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

		void Draw::beginDraw(Engine* pEngine) {
			this->_isBegin = false;

			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);
			
			if (!pPresentInfo) return;

			if (!this->_isInit) {
				this->_isInit = this->initialize(pEngine);

				if (!this->_isInit) return;
			}

			const VkSwapchainKHR hSwapchain = pPresentInfo->pSwapchains[0];
			uint32_t curImageCount = 0u;

			if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &curImageCount, nullptr) != VkResult::VK_SUCCESS) return;

			if (!curImageCount) return;

			if (!this->resizeImageDataArray(hSwapchain, curImageCount)) {
				this->destroyImageDataArray();

				return;
			}

			this->_pCurImageData = &this->_pImageDataArray[pPresentInfo->pImageIndices[0]];

			this->_f.pVkWaitForFences(this->_hDevice, 1u, &this->_pCurImageData->hFence, VK_TRUE, ~0ull);
			this->_f.pVkResetFences(this->_hDevice, 1u, &this->_pCurImageData->hFence);

			RECT curWindowRect{};

			if (!GetClientRect(this->_hMainWindow, &curWindowRect)) return;

			if (curWindowRect.right != this->_windowRect.right || curWindowRect.bottom != this->_windowRect.bottom) {
				this->destroyFramebuffers();

				if (!this->createFramebuffers(hSwapchain)) return;
				
				this->_windowRect = curWindowRect;
				pEngine->setWindowSize(static_cast<int>(this->_windowRect.right), static_cast<int>(this->_windowRect.bottom));
			}

			if (!this->mapBufferData(&this->_pCurImageData->triangleListBufferData)) return;
			
			if (!this->mapBufferData(&this->_pCurImageData->pointListBufferData)) return;

			if (!this->beginCommandBuffer(this->_pCurImageData->hCommandBuffer)) return;

			this->beginRenderPass(this->_pCurImageData->hCommandBuffer, this->_pCurImageData->hFrameBuffer);

			this->_isBegin = true;

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			const VkQueue hQueue = reinterpret_cast<VkQueue>(pEngine->pHookArg1);
			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);

			if (!this->_isBegin || !pPresentInfo) return;

			const VkViewport viewport{ 0.f, 0.f, static_cast<float>(this->_windowRect.right), static_cast<float>(this->_windowRect.bottom), 0.f, 1.f };
			this->_f.pVkCmdSetViewport(this->_pCurImageData->hCommandBuffer, 0u, 1u, &viewport);

			const VkRect2D scissor{ { 0, 0 }, { static_cast<uint32_t>(this->_windowRect.right), static_cast<uint32_t>(this->_windowRect.bottom) } };
			this->_f.pVkCmdSetScissor(this->_pCurImageData->hCommandBuffer, 0u, 1u, &scissor);

			const float scale[]{ 2.f / static_cast<float>(this->_windowRect.right), 2.f / static_cast<float>(this->_windowRect.bottom) };
			this->_f.pVkCmdPushConstants(this->_pCurImageData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(scale), scale);
			
			const float translate[2]{ -1.f, -1.f };
			this->_f.pVkCmdPushConstants(this->_pCurImageData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(scale), sizeof(translate), translate);

			this->_f.pVkCmdBindPipeline(this->_pCurImageData->hCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hTriangleListPipeline);
			this->drawBufferData(&this->_pCurImageData->triangleListBufferData, this->_pCurImageData->hCommandBuffer);

			this->_f.pVkCmdBindPipeline(this->_pCurImageData->hCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hPointListPipeline);
			this->drawBufferData(&this->_pCurImageData->pointListBufferData, this->_pCurImageData->hCommandBuffer);

			this->_f.pVkCmdEndRenderPass(this->_pCurImageData->hCommandBuffer);
			this->_f.pVkEndCommandBuffer(this->_pCurImageData->hCommandBuffer);

			if (hQueue != this->_hFirstGraphicsQueue) return;

			VkPipelineStageFlags* const pStageMask = new VkPipelineStageFlags[pPresentInfo->waitSemaphoreCount];
			memset(pStageMask, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, pPresentInfo->waitSemaphoreCount * sizeof(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT));
			
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pWaitDstStageMask = pStageMask;
			submitInfo.commandBufferCount = 1u;
			submitInfo.pCommandBuffers = &this->_pCurImageData->hCommandBuffer;
			submitInfo.pWaitSemaphores = pPresentInfo->pWaitSemaphores;
			submitInfo.waitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
			submitInfo.pSignalSemaphores = pPresentInfo->pWaitSemaphores;
			submitInfo.signalSemaphoreCount = pPresentInfo->waitSemaphoreCount;

			this->_f.pVkQueueSubmit(hQueue, 1u, &submitInfo, this->_pCurImageData->hFence);

			delete[] pStageMask;

			return;
		}

		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit || count % 3) return;

			this->copyToBufferData(&this->_pCurImageData->triangleListBufferData, corners, count, color);

			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {
			
			if (!this->_isBegin || !pFont) return;

			const font::Font* const pCurFont = reinterpret_cast<const font::Font*>(pFont);

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;

				const font::CharIndex index = font::charToCharIndex(c);
				const font::Char* pCurChar = &pCurFont->chars[index];

				if (pCurChar) {
					// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
					const Vector2 curPos{ pos->x + (pCurFont->width + 2.f) * i, pos->y - pCurFont->height };
					this->copyToBufferData(&this->_pCurImageData->pointListBufferData, pCurChar->body.coordinates, pCurChar->body.count, color, curPos);
					this->copyToBufferData(&this->_pCurImageData->pointListBufferData, pCurChar->outline.coordinates, pCurChar->outline.count, rgb::black, curPos);
				}

			}

			return;
		}


		#define ASSIGN_PROC_ADDRESS(dispatchableObject, f) this->_f.pVk##f = reinterpret_cast<PFN_vk##f>(pVkGet##dispatchableObject##ProcAddress(this->_h##dispatchableObject, "vk"#f))

		bool Draw::initialize(const Engine* pEngine) {
			this->_hVulkan = proc::in::getModuleHandle("vulkan-1.dll");

			if (!this->_hVulkan) return false;
			
			EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&this->_hMainWindow));

			if (!this->_hMainWindow) return false;
			
			this->_hDevice = reinterpret_cast<VkDevice>(pEngine->pHookArg3);

			if (this->_hDevice == VK_NULL_HANDLE) return false;

			if (this->_hInstance == VK_NULL_HANDLE) {
				this->_hInstance = createInstance(this->_hVulkan);
			}

			if (this->_hInstance == VK_NULL_HANDLE) return false;

			if (!this->getProcAddresses()) return false;

			this->_hPhysicalDevice = getPhysicalDevice(this->_hVulkan, this->_hInstance);

			if (this->_hPhysicalDevice == VK_NULL_HANDLE) return false;

			this->_graphicsQueueFamilyIndex = getGraphicsQueueFamilyIndex(this->_hVulkan, this->_hPhysicalDevice);

			if (this->_graphicsQueueFamilyIndex == 0xFFFFFFFF) return false;

			if (this->_hRenderPass == VK_NULL_HANDLE) {

				if (!this->createRenderPass()) return false;

			}

			if (this->_hCommandPool == VK_NULL_HANDLE) {
				
				if (!this->createCommandPool()) return false;

			}

			if (this->_hTriangleListPipeline == VK_NULL_HANDLE) {
				this->_hTriangleListPipeline = this->createPipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
			}

			if (this->_hTriangleListPipeline == VK_NULL_HANDLE) return false;

			if (this->_hPointListPipeline == VK_NULL_HANDLE) {
				this->_hPointListPipeline = this->createPipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
			}

			if (this->_hPointListPipeline == VK_NULL_HANDLE) return false;

			this->_f.pVkGetPhysicalDeviceMemoryProperties(this->_hPhysicalDevice, &this->_memoryProperties);

			if (!this->_memoryProperties.memoryTypeCount) return false;

			this->_f.pVkGetDeviceQueue(this->_hDevice, this->_graphicsQueueFamilyIndex, 0u, &this->_hFirstGraphicsQueue);

			if (this->_hFirstGraphicsQueue == VK_NULL_HANDLE) return false;
			
			return true;
		}

		bool Draw::getProcAddresses() {
			const PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddress = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetDeviceProcAddr"));
			const PFN_vkGetInstanceProcAddr pVkGetInstanceProcAddress = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetInstanceProcAddr"));
			
			if (!pVkGetDeviceProcAddress || !pVkGetInstanceProcAddress) return false;

			ASSIGN_PROC_ADDRESS(Instance, GetPhysicalDeviceMemoryProperties);
			ASSIGN_PROC_ADDRESS(Device, GetSwapchainImagesKHR);
			ASSIGN_PROC_ADDRESS(Device, CreateCommandPool);
			ASSIGN_PROC_ADDRESS(Device, DestroyCommandPool);
			ASSIGN_PROC_ADDRESS(Device, AllocateCommandBuffers);
			ASSIGN_PROC_ADDRESS(Device, FreeCommandBuffers);
			ASSIGN_PROC_ADDRESS(Device, CreateRenderPass);
			ASSIGN_PROC_ADDRESS(Device, DestroyRenderPass);
			ASSIGN_PROC_ADDRESS(Device, CreateShaderModule);
			ASSIGN_PROC_ADDRESS(Device, DestroyShaderModule);
			ASSIGN_PROC_ADDRESS(Device, CreatePipelineLayout);
			ASSIGN_PROC_ADDRESS(Device, DestroyPipelineLayout);
			ASSIGN_PROC_ADDRESS(Device, CreateDescriptorSetLayout);
			ASSIGN_PROC_ADDRESS(Device, DestroyDescriptorSetLayout);
			ASSIGN_PROC_ADDRESS(Device, CreateGraphicsPipelines);
			ASSIGN_PROC_ADDRESS(Device, DestroyPipeline);
			ASSIGN_PROC_ADDRESS(Device, CreateBuffer);
			ASSIGN_PROC_ADDRESS(Device, DestroyBuffer);
			ASSIGN_PROC_ADDRESS(Device, GetBufferMemoryRequirements);
			ASSIGN_PROC_ADDRESS(Device, AllocateMemory);
			ASSIGN_PROC_ADDRESS(Device, BindBufferMemory);
			ASSIGN_PROC_ADDRESS(Device, FreeMemory);
			ASSIGN_PROC_ADDRESS(Device, CreateImageView);
			ASSIGN_PROC_ADDRESS(Device, DestroyImageView);
			ASSIGN_PROC_ADDRESS(Device, CreateFramebuffer);
			ASSIGN_PROC_ADDRESS(Device, DestroyFramebuffer);
			ASSIGN_PROC_ADDRESS(Device, CreateFence);
			ASSIGN_PROC_ADDRESS(Device, DestroyFence);
			ASSIGN_PROC_ADDRESS(Device, WaitForFences);
			ASSIGN_PROC_ADDRESS(Device, ResetFences);
			ASSIGN_PROC_ADDRESS(Device, ResetCommandBuffer);
			ASSIGN_PROC_ADDRESS(Device, BeginCommandBuffer);
			ASSIGN_PROC_ADDRESS(Device, EndCommandBuffer);
			ASSIGN_PROC_ADDRESS(Device, CmdBeginRenderPass);
			ASSIGN_PROC_ADDRESS(Device, CmdEndRenderPass);
			ASSIGN_PROC_ADDRESS(Device, MapMemory);
			ASSIGN_PROC_ADDRESS(Device, UnmapMemory);
			ASSIGN_PROC_ADDRESS(Device, FlushMappedMemoryRanges);
			ASSIGN_PROC_ADDRESS(Device, CmdBindPipeline);
			ASSIGN_PROC_ADDRESS(Device, CmdBindVertexBuffers);
			ASSIGN_PROC_ADDRESS(Device, CmdBindIndexBuffer);
			ASSIGN_PROC_ADDRESS(Device, CmdSetViewport);
			ASSIGN_PROC_ADDRESS(Device, CmdPushConstants);
			ASSIGN_PROC_ADDRESS(Device, CmdSetScissor);
			ASSIGN_PROC_ADDRESS(Device, CmdDrawIndexed);
			ASSIGN_PROC_ADDRESS(Device, GetDeviceQueue);
			ASSIGN_PROC_ADDRESS(Device, QueueSubmit);

			for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {
				
				if (!(this->_fPtrs[i])) return false;

			}

			return true;
		}


		bool Draw::createRenderPass() {
			VkAttachmentDescription attachmentDesc{};
			attachmentDesc.format = VK_FORMAT_B8G8R8A8_UNORM;
			attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachment{};
			colorAttachment.attachment = 0u;
			colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkSubpassDescription subpassDesc{};
			subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDesc.colorAttachmentCount = 1u;
			subpassDesc.pColorAttachments = &colorAttachment;

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1u;
			renderPassCreateInfo.pAttachments = &attachmentDesc;
			renderPassCreateInfo.subpassCount = 1u;
			renderPassCreateInfo.pSubpasses = &subpassDesc;

			return this->_f.pVkCreateRenderPass(this->_hDevice, &renderPassCreateInfo, nullptr, &this->_hRenderPass) == VkResult::VK_SUCCESS;
		}


		bool Draw::createCommandPool() {
			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			commandPoolCreateInfo.queueFamilyIndex = this->_graphicsQueueFamilyIndex;

			return this->_f.pVkCreateCommandPool(this->_hDevice, &commandPoolCreateInfo, nullptr, &this->_hCommandPool) == VkResult::VK_SUCCESS;
		}


		/*
		#version 450 core
		layout(location = 0) in vec2 inPosition;
		layout(location = 1) in vec4 inColor;

		layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

		out gl_PerVertex { vec4 gl_Position; };
		layout(location = 0) out vec4 fragColor;

		void main()
		{
			gl_Position = vec4(inPosition * pc.uScale + pc.uTranslate, 0, 1);
			fragColor = inColor;
		}
		*/
		static constexpr BYTE GLSL_SHADER_VERT[]{
			0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x0d, 0x00, 0x27, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
			0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x0f, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
			0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
			0x25, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xc2, 0x01, 0x00, 0x00,
			0x04, 0x00, 0x0a, 0x00, 0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x63, 0x70,
			0x70, 0x5f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x5f, 0x6c, 0x69, 0x6e, 0x65, 0x5f, 0x64, 0x69, 0x72,
			0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00, 0x47, 0x4c, 0x5f, 0x47,
			0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64, 0x65, 0x5f, 0x64, 0x69,
			0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x05, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00,
			0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00,
			0x67, 0x6c, 0x5f, 0x50, 0x65, 0x72, 0x56, 0x65, 0x72, 0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00,
			0x06, 0x00, 0x06, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0x6c, 0x5f, 0x50,
			0x6f, 0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x05, 0x00, 0x03, 0x00, 0x0a, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x50, 0x6f,
			0x73, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x00, 0x00, 0x05, 0x00, 0x06, 0x00, 0x11, 0x00, 0x00, 0x00,
			0x75, 0x50, 0x75, 0x73, 0x68, 0x43, 0x6f, 0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x00, 0x00, 0x00,
			0x06, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x53, 0x63, 0x61,
			0x6c, 0x65, 0x00, 0x00, 0x06, 0x00, 0x06, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x75, 0x54, 0x72, 0x61, 0x6e, 0x73, 0x6c, 0x61, 0x74, 0x65, 0x00, 0x00, 0x05, 0x00, 0x03, 0x00,
			0x13, 0x00, 0x00, 0x00, 0x70, 0x63, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00, 0x23, 0x00, 0x00, 0x00,
			0x66, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
			0x25, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x48, 0x00, 0x05, 0x00,
			0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x47, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00,
			0x0f, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x05, 0x00,
			0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x48, 0x00, 0x05, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00,
			0x08, 0x00, 0x00, 0x00, 0x47, 0x00, 0x03, 0x00, 0x11, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
			0x47, 0x00, 0x04, 0x00, 0x23, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x47, 0x00, 0x04, 0x00, 0x25, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
			0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
			0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
			0x1e, 0x00, 0x03, 0x00, 0x08, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
			0x09, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
			0x09, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00,
			0x0b, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
			0x0b, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00,
			0x0d, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
			0x0e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
			0x0e, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x04, 0x00,
			0x11, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
			0x12, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
			0x12, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
			0x14, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
			0x0b, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
			0x06, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x00, 0x04, 0x00,
			0x06, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x20, 0x00, 0x04, 0x00,
			0x21, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
			0x21, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
			0x24, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
			0x24, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00,
			0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
			0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00,
			0x10, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00, 0x14, 0x00, 0x00, 0x00,
			0x15, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
			0x0d, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x15, 0x00, 0x00, 0x00, 0x85, 0x00, 0x05, 0x00,
			0x0d, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
			0x41, 0x00, 0x05, 0x00, 0x14, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
			0x18, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
			0x19, 0x00, 0x00, 0x00, 0x81, 0x00, 0x05, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00,
			0x17, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00,
			0x1e, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x00, 0x05, 0x00,
			0x06, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x50, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
			0x1f, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1d, 0x00, 0x00, 0x00, 0x41, 0x00, 0x05, 0x00,
			0x21, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00,
			0x3e, 0x00, 0x03, 0x00, 0x22, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00,
			0x07, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
			0x23, 0x00, 0x00, 0x00, 0x26, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00, 0x38, 0x00, 0x01, 0x00
		};

		/*
		#version 450
		layout(location = 0) in vec4 fragColor;
		layout(location = 0) out vec4 outColor;

		void main() {
			outColor = fragColor;
		}
		*/
		static constexpr BYTE GLSL_SHADER_FRAG[]{
			0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0b, 0x00, 0x0d, 0x00, 0x0d, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
			0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e,
			0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
			0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00,
			0xc2, 0x01, 0x00, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c,
			0x45, 0x5f, 0x63, 0x70, 0x70, 0x5f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x5f, 0x6c, 0x69, 0x6e, 0x65,
			0x5f, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x00, 0x04, 0x00, 0x08, 0x00,
			0x47, 0x4c, 0x5f, 0x47, 0x4f, 0x4f, 0x47, 0x4c, 0x45, 0x5f, 0x69, 0x6e, 0x63, 0x6c, 0x75, 0x64,
			0x65, 0x5f, 0x64, 0x69, 0x72, 0x65, 0x63, 0x74, 0x69, 0x76, 0x65, 0x00, 0x05, 0x00, 0x04, 0x00,
			0x04, 0x00, 0x00, 0x00, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x05, 0x00,
			0x09, 0x00, 0x00, 0x00, 0x6f, 0x75, 0x74, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x00, 0x00, 0x00,
			0x05, 0x00, 0x05, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x66, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6c, 0x6f,
			0x72, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x09, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00,
			0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00,
			0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
			0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
			0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00,
			0x03, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
			0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
			0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00, 0x05, 0x00, 0x00, 0x00,
			0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
			0x3e, 0x00, 0x03, 0x00, 0x09, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
			0x38, 0x00, 0x01, 0x00
		};

		VkPipeline Draw::createPipeline(VkPrimitiveTopology topology) {

			if (this->_hShaderModuleVert == VK_NULL_HANDLE) {

				this->_hShaderModuleVert = this->createShaderModule(GLSL_SHADER_VERT, sizeof(GLSL_SHADER_VERT));

			}

			if (this->_hShaderModuleVert == VK_NULL_HANDLE) return VK_NULL_HANDLE;

			if (this->_hShaderModuleFrag == VK_NULL_HANDLE) {

				this->_hShaderModuleFrag = this->createShaderModule(GLSL_SHADER_FRAG, sizeof(GLSL_SHADER_FRAG));

			}

			if (this->_hShaderModuleFrag == VK_NULL_HANDLE) return VK_NULL_HANDLE;
			
			if (this->_hPipelineLayout == VK_NULL_HANDLE) {

				if (!this->createPipelineLayout()) return VK_NULL_HANDLE;

			}

			VkPipelineShaderStageCreateInfo stageCreateInfo[2]{};
			stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			stageCreateInfo[0].module = this->_hShaderModuleVert;
			stageCreateInfo[0].pName = "main";
			stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			stageCreateInfo[1].module = this->_hShaderModuleFrag;
			stageCreateInfo[1].pName = "main";

			VkVertexInputBindingDescription bindingDesc{};
			bindingDesc.stride = sizeof(Vertex);
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription attributeDesc[2]{};
			attributeDesc[0].location = 0u;
			attributeDesc[0].binding = bindingDesc.binding;
			attributeDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDesc[0].offset = 0u;
			attributeDesc[1].location = 1u;
			attributeDesc[1].binding = bindingDesc.binding;
			attributeDesc[1].format = VK_FORMAT_B8G8R8A8_UNORM;
			attributeDesc[1].offset = sizeof(Vector2);

			VkPipelineVertexInputStateCreateInfo vertexInfo{};
			vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInfo.vertexBindingDescriptionCount = 1u;
			vertexInfo.pVertexBindingDescriptions = &bindingDesc;
			vertexInfo.vertexAttributeDescriptionCount = 2u;
			vertexInfo.pVertexAttributeDescriptions = attributeDesc;

			VkPipelineInputAssemblyStateCreateInfo iaInfo{};
			iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			iaInfo.topology = topology;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.viewportCount = 1u;
			viewportInfo.scissorCount = 1u;

			VkPipelineRasterizationStateCreateInfo rasterInfo{};
			rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterInfo.cullMode = VK_CULL_MODE_NONE;
			rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterInfo.lineWidth = 1.f;

			VkPipelineMultisampleStateCreateInfo multisampleInfo{};
			multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorAttachment{};
			colorAttachment.blendEnable = VK_TRUE;
			colorAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

			VkPipelineDepthStencilStateCreateInfo depthInfo{};
			depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.attachmentCount = 1u;
			blendInfo.pAttachments = &colorAttachment;

			constexpr VkDynamicState dynamicStates[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = _countof(dynamicStates);
			dynamicState.pDynamicStates = dynamicStates;

			VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.flags = 0u;
			pipelineCreateInfo.stageCount = 2u;
			pipelineCreateInfo.pStages = stageCreateInfo;
			pipelineCreateInfo.pVertexInputState = &vertexInfo;
			pipelineCreateInfo.pInputAssemblyState = &iaInfo;
			pipelineCreateInfo.pViewportState = &viewportInfo;
			pipelineCreateInfo.pRasterizationState = &rasterInfo;
			pipelineCreateInfo.pMultisampleState = &multisampleInfo;
			pipelineCreateInfo.pDepthStencilState = &depthInfo;
			pipelineCreateInfo.pColorBlendState = &blendInfo;
			pipelineCreateInfo.pDynamicState = &dynamicState;
			pipelineCreateInfo.layout = this->_hPipelineLayout;
			pipelineCreateInfo.renderPass = this->_hRenderPass;
			pipelineCreateInfo.subpass = 0u;

			VkPipeline hPipeline{};

			if (this->_f.pVkCreateGraphicsPipelines(this->_hDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &hPipeline) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hPipeline;
		}


		VkShaderModule Draw::createShaderModule(const BYTE shader[], size_t size) const {
			VkShaderModuleCreateInfo fragCreateInfo{};
			fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragCreateInfo.codeSize = size;
			fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shader);

			VkShaderModule hShaderModule{};

			if (this->_f.pVkCreateShaderModule(this->_hDevice, &fragCreateInfo, nullptr, &hShaderModule) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			return hShaderModule;
		}


		bool Draw::createPipelineLayout() {

			if (this->_hDescriptorSetLayout == VK_NULL_HANDLE) {
					
				if (!this->createDescriptorSetLayout()) return false;

			}

			VkPushConstantRange pushConstants{};
			pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstants.offset = 0u;
			pushConstants.size = sizeof(float) * 4u;
			
			VkPipelineLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 1u;
			layoutCreateInfo.pSetLayouts = &this->_hDescriptorSetLayout;
			layoutCreateInfo.pushConstantRangeCount = 1u;
			layoutCreateInfo.pPushConstantRanges = &pushConstants;
				
			return this->_f.pVkCreatePipelineLayout(this->_hDevice, &layoutCreateInfo, nullptr, &this->_hPipelineLayout) == VkResult::VK_SUCCESS;
		}


		bool Draw::createDescriptorSetLayout() {
			VkDescriptorSetLayoutBinding binding{};
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutCreateInfo descCreateinfo{};
			descCreateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descCreateinfo.bindingCount = 1;
			descCreateinfo.pBindings = &binding;

			return this->_f.pVkCreateDescriptorSetLayout(this->_hDevice, &descCreateinfo, nullptr, &this->_hDescriptorSetLayout) == VkResult::VK_SUCCESS;
		}


		bool Draw::resizeImageDataArray(VkSwapchainKHR hSwapchain, uint32_t imageCount) {

			if (imageCount < this->_imageCount) {

				for (uint32_t i = this->_imageCount; i >= imageCount; i--) {
					this->destroyImageData(&this->_pImageDataArray[i]);
				}

				this->_imageCount = imageCount;

				return true;
			}

			if (imageCount == this->_imageCount) return true;

			ImageData* const pOldImageData = this->_pImageDataArray;
			uint32_t oldImageCount = this->_imageCount;
			
			this->_pImageDataArray = new ImageData[imageCount]{};
			this->_imageCount = imageCount;
			
			VkCommandBufferAllocateInfo commandBufferAllocInfo{};
			commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocInfo.commandPool = this->_hCommandPool;
			commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocInfo.commandBufferCount = 1u;

			VkFenceCreateInfo fenceCreateInfo{};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (uint32_t i = 0; i < this->_imageCount; i++) {

				if (pOldImageData && i < oldImageCount) {
					memcpy(&this->_pImageDataArray[i], &pOldImageData[i], sizeof(ImageData));
				}
				else {

					if (this->_f.pVkAllocateCommandBuffers(this->_hDevice, &commandBufferAllocInfo, &this->_pImageDataArray[i].hCommandBuffer) != VkResult::VK_SUCCESS) return false;

					if (this->_f.pVkCreateFence(this->_hDevice, &fenceCreateInfo, nullptr, &this->_pImageDataArray[i].hFence) != VkResult::VK_SUCCESS) return false;

					static constexpr size_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

					if (!this->createBufferData(&this->_pImageDataArray[i].triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

					static constexpr size_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;

					if (!this->createBufferData(&this->_pImageDataArray[i].pointListBufferData, INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) return false;

				}

			}

			this->destroyFramebuffers();

			return this->createFramebuffers(hSwapchain);
		}


		void Draw::destroyImageDataArray() {

			if (!this->_pImageDataArray) return;

			for (uint32_t i = 0u; i < this->_imageCount && this->_hDevice != VK_NULL_HANDLE; i++) {
				this->destroyImageData(&this->_pImageDataArray[i]);
			}

			delete[] this->_pImageDataArray;
			this->_pImageDataArray = nullptr;
			this->_imageCount = 0u;

			return;
		}


		void Draw::destroyImageData(ImageData* pImageData) const {

			if (pImageData->hCommandBuffer != VK_NULL_HANDLE) {
				this->_f.pVkFreeCommandBuffers(this->_hDevice, this->_hCommandPool, 1u, &pImageData->hCommandBuffer);
				pImageData->hCommandBuffer = VK_NULL_HANDLE;
			}

			if (pImageData->hFrameBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyFramebuffer(this->_hDevice, pImageData->hFrameBuffer, nullptr);
				pImageData->hFrameBuffer = VK_NULL_HANDLE;
			}

			if (pImageData->hImageView != VK_NULL_HANDLE) {
				this->_f.pVkDestroyImageView(this->_hDevice, pImageData->hImageView, nullptr);
				pImageData->hImageView = VK_NULL_HANDLE;
			}

			this->destroyBufferData(&pImageData->triangleListBufferData);
			this->destroyBufferData(&pImageData->pointListBufferData);

			if (pImageData->hFence != VK_NULL_HANDLE) {
				this->_f.pVkDestroyFence(this->_hDevice, pImageData->hFence, nullptr);
				pImageData->hFence = VK_NULL_HANDLE;
			}

			return;
		}


		bool Draw::createFramebuffers(VkSwapchainKHR hSwapchain) {
			VkImage* const pImages = new VkImage[this->_imageCount]{};

			uint32_t imageCount = this->_imageCount;

			if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &imageCount, pImages) != VkResult::VK_SUCCESS || imageCount != this->_imageCount) {
				delete[] pImages;

				return false;
			}
			
			VkImageSubresourceRange imageSubresourceRange{};
			imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageSubresourceRange.baseMipLevel = 0u;
			imageSubresourceRange.levelCount = 1u;
			imageSubresourceRange.baseArrayLayer = 0u;
			imageSubresourceRange.layerCount = 1u;

			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
			imageViewCreateInfo.subresourceRange = imageSubresourceRange;

			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = this->_hRenderPass;
			framebufferCreateInfo.attachmentCount = 1u;
			framebufferCreateInfo.layers = 1u;

			for (uint32_t i = 0; i < this->_imageCount; i++) {				
				imageViewCreateInfo.image = pImages[i];

				if (this->_f.pVkCreateImageView(this->_hDevice, &imageViewCreateInfo, nullptr, &this->_pImageDataArray[i].hImageView) != VkResult::VK_SUCCESS) {
					delete[] pImages;
					
					return false;
				}

				framebufferCreateInfo.pAttachments = &this->_pImageDataArray[i].hImageView;

				if (this->_f.pVkCreateFramebuffer(this->_hDevice, &framebufferCreateInfo, nullptr, &this->_pImageDataArray[i].hFrameBuffer) != VkResult::VK_SUCCESS) {
					delete[] pImages;

					return false;
				}

			}

			delete[] pImages;

			return true;
		}


		void Draw::destroyFramebuffers() {
			
			for (uint32_t i = 0u; i < this->_imageCount; i++) {
				
				if (this->_pImageDataArray[i].hFrameBuffer != VK_NULL_HANDLE) {
					this->_f.pVkDestroyFramebuffer(this->_hDevice, this->_pImageDataArray[i].hFrameBuffer, nullptr);
					this->_pImageDataArray[i].hFrameBuffer = VK_NULL_HANDLE;
				}

				if (this->_pImageDataArray[i].hImageView != VK_NULL_HANDLE) {
					this->_f.pVkDestroyImageView(this->_hDevice, this->_pImageDataArray[i].hImageView, nullptr);
					this->_pImageDataArray[i].hImageView = VK_NULL_HANDLE;
				}

			}
			
			return;
		}


		bool Draw::createBufferData(BufferData* pBufferData, size_t vertexCount) {
			RtlSecureZeroMemory(pBufferData, sizeof(BufferData));

			VkDeviceSize vertexBufferSize = vertexCount * sizeof(Vertex);

			if (!this->createBuffer(&pBufferData->hVertexBuffer, &pBufferData->hVertexMemory, &vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

			pBufferData->vertexBufferSize = vertexBufferSize;

			VkDeviceSize indexBufferSize = vertexCount * sizeof(uint32_t);

			if (!this->createBuffer(&pBufferData->hIndexBuffer, &pBufferData->hIndexMemory, &indexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

			pBufferData->indexBufferSize = indexBufferSize;

			return true;
		}


		void Draw::destroyBufferData(BufferData* pBufferData) const {

			if (pBufferData->hVertexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, pBufferData->hVertexBuffer, nullptr);
				pBufferData->hVertexBuffer = VK_NULL_HANDLE;
			}

			if (pBufferData->hVertexMemory != VK_NULL_HANDLE) {
				this->_f.pVkUnmapMemory(this->_hDevice, pBufferData->hVertexMemory);
				this->_f.pVkFreeMemory(this->_hDevice, pBufferData->hVertexMemory, nullptr);
				pBufferData->hVertexMemory = VK_NULL_HANDLE;
			}

			if (pBufferData->hIndexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, pBufferData->hIndexBuffer, nullptr);
				pBufferData->hIndexBuffer = VK_NULL_HANDLE;
			}

			if (pBufferData->hIndexMemory != VK_NULL_HANDLE) {
				this->_f.pVkUnmapMemory(this->_hDevice, pBufferData->hIndexMemory);
				this->_f.pVkFreeMemory(this->_hDevice, pBufferData->hIndexMemory, nullptr);
				pBufferData->hIndexMemory = VK_NULL_HANDLE;
			}

			pBufferData->vertexBufferSize = 0ull;
			pBufferData->pLocalVertexBuffer = nullptr;
			pBufferData->indexBufferSize = 0ull;
			pBufferData->pLocalIndexBuffer = nullptr;
			pBufferData->curOffset = 0u;

			return;
		}


		bool Draw::createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, VkDeviceSize* pSize, VkBufferUsageFlagBits usage) {
			const VkDeviceSize sizeAligned = (((*pSize) - 1ull) / this->_bufferAlignment + 1ull) * this->_bufferAlignment;

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

			*pSize = memoryReq.size;

			return true;
		}


		uint32_t Draw::getMemoryTypeIndex(uint32_t typeBits) const {

			for (uint32_t i = 0u; i < this->_memoryProperties.memoryTypeCount; i++) {

				if ((_memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && typeBits & (1 << i)) {
					return i;
				}
			}


			return 0xFFFFFFFF;
		}


		bool Draw::mapBufferData(BufferData* pBufferData) const {

			if (!pBufferData->pLocalVertexBuffer) {

				if (this->_f.pVkMapMemory(this->_hDevice, pBufferData->hVertexMemory, 0ull, pBufferData->vertexBufferSize, 0ull, reinterpret_cast<void**>(&pBufferData->pLocalVertexBuffer)) != VkResult::VK_SUCCESS) return false;

			}

			if (!pBufferData->pLocalIndexBuffer) {

				if (this->_f.pVkMapMemory(this->_hDevice, pBufferData->hIndexMemory, 0ull, pBufferData->indexBufferSize, 0ull, reinterpret_cast<void**>(&pBufferData->pLocalIndexBuffer)) != VkResult::VK_SUCCESS) return false;

			}

			return true;
		}


		bool Draw::beginCommandBuffer(VkCommandBuffer hCommandBuffer) const {
			
			if (this->_f.pVkResetCommandBuffer(hCommandBuffer, 0u) != VkResult::VK_SUCCESS) return false;

			VkCommandBufferBeginInfo cmdBufferBeginInfo{};
			cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			return this->_f.pVkBeginCommandBuffer(hCommandBuffer, &cmdBufferBeginInfo) == VkResult::VK_SUCCESS;
		}


		void Draw::beginRenderPass(VkCommandBuffer hCommandBuffer, VkFramebuffer hFramebuffer) const {
			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = this->_hRenderPass;
			renderPassBeginInfo.framebuffer = hFramebuffer;
			renderPassBeginInfo.renderArea.extent.width = this->_windowRect.right;
			renderPassBeginInfo.renderArea.extent.height = this->_windowRect.bottom;
			this->_f.pVkCmdBeginRenderPass(hCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			return;
		}


		void Draw::copyToBufferData(BufferData* pBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) {

			if (!pBufferData->pLocalVertexBuffer || !pBufferData->pLocalIndexBuffer) return;

			const size_t newVertexCount = pBufferData->curOffset + count;

			if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize || newVertexCount * sizeof(uint32_t) > pBufferData->indexBufferSize) {

				if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

			}

			for (UINT i = 0u; i < count; i++) {
				const uint32_t curIndex = pBufferData->curOffset + i;

				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
				memcpy(&(pBufferData->pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

				pBufferData->pLocalIndexBuffer[curIndex] = curIndex;
			}

			pBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeBufferData(BufferData* pBufferData, size_t newVertexCount) {

			if (newVertexCount <= pBufferData->curOffset) return true;

			BufferData oldBufferData = *pBufferData;

			if (!this->createBufferData(pBufferData, newVertexCount)) {
				this->destroyBufferData(pBufferData);
				
				return false;
			}

			if (this->_f.pVkMapMemory(this->_hDevice, pBufferData->hVertexMemory, 0ull, pBufferData->vertexBufferSize, 0ull, reinterpret_cast<void**>(&pBufferData->pLocalVertexBuffer)) != VkResult::VK_SUCCESS) return false;

			if (oldBufferData.pLocalVertexBuffer) {
				memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, static_cast<size_t>(oldBufferData.vertexBufferSize));
			}

			if (this->_f.pVkMapMemory(this->_hDevice, pBufferData->hIndexMemory, 0ull, pBufferData->indexBufferSize, 0ull, reinterpret_cast<void**>(&pBufferData->pLocalIndexBuffer)) != VkResult::VK_SUCCESS) return false;

			if (oldBufferData.pLocalIndexBuffer) {
				memcpy(pBufferData->pLocalIndexBuffer, oldBufferData.pLocalIndexBuffer, static_cast<size_t>(oldBufferData.indexBufferSize));
			}

			pBufferData->curOffset = oldBufferData.curOffset;

			this->destroyBufferData(&oldBufferData);

			return true;
		}


		void Draw::drawBufferData(BufferData* pBufferData, VkCommandBuffer hCommandBuffer) const {
			VkMappedMemoryRange ranges[2]{};
			ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			ranges[0].memory = pBufferData->hVertexMemory;
			ranges[0].size = VK_WHOLE_SIZE;
			ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			ranges[1].memory = pBufferData->hIndexMemory;
			ranges[1].size = VK_WHOLE_SIZE;

			if (this->_f.pVkFlushMappedMemoryRanges(this->_hDevice, _countof(ranges), ranges) != VkResult::VK_SUCCESS) return;

			this->_f.pVkUnmapMemory(this->_hDevice, pBufferData->hVertexMemory);
			pBufferData->pLocalVertexBuffer = nullptr;

			this->_f.pVkUnmapMemory(this->_hDevice, pBufferData->hIndexMemory);
			pBufferData->pLocalIndexBuffer = nullptr;
			
			constexpr VkDeviceSize offset = 0ull;
			this->_f.pVkCmdBindVertexBuffers(hCommandBuffer, 0u, 1u, &pBufferData->hVertexBuffer, &offset);
			this->_f.pVkCmdBindIndexBuffer(hCommandBuffer, pBufferData->hIndexBuffer, 0ull, VK_INDEX_TYPE_UINT32);
			this->_f.pVkCmdDrawIndexed(hCommandBuffer, pBufferData->curOffset, 1u, 0u, 0u, 0u);

			pBufferData->curOffset = 0u;

			return;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0ul;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;

			char className[MAX_PATH]{};

			if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

			if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}

	}

}