#include "vkBackend.h"
#include "..\Vertex.h"
#include "..\font\Font.h"
#include "..\..\proc.h"
#include "..\..\hooks\TrampHook.h"

namespace hax {

	namespace draw {

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

			bool getInitData(InitData* initData) {
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

				hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

				if (!hHookSemaphore) return false;

				pAcquireHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(pVkAcquireNextImageKHR), reinterpret_cast<BYTE*>(hkvkAcquireNextImageKHR), 0xCu);
				
				if (!pAcquireHook->enable()) {
					delete pAcquireHook;
					CloseHandle(hHookSemaphore);

					return false;
				}

				WaitForSingleObject(hHookSemaphore, INFINITE);
				CloseHandle(hHookSemaphore);
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
			
				uint32_t tmpGpuCount{};

				if (pVkEnumeratePhysicalDevices(hInstance, &tmpGpuCount, nullptr) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

				if (!tmpGpuCount) return VK_NULL_HANDLE;

				const uint32_t gpuCount = tmpGpuCount;
				VkPhysicalDevice* const pPhysicalDeviceArray = new VkPhysicalDevice[gpuCount]{};

				if (pVkEnumeratePhysicalDevices(hInstance, &tmpGpuCount, pPhysicalDeviceArray) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

				if (tmpGpuCount != gpuCount) return VK_NULL_HANDLE;

				VkPhysicalDevice hPhysicalDevice = VK_NULL_HANDLE;

				for (uint32_t i = 0u; i < gpuCount; i++) {
					VkPhysicalDeviceProperties properties{};
					pVkGetPhysicalDeviceProperties(pPhysicalDeviceArray[i], &properties);

					if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
						hPhysicalDevice = pPhysicalDeviceArray[i];
						break;
					}
				}

				delete[] pPhysicalDeviceArray;

				return hPhysicalDevice;
			}


			static uint32_t getGraphicsQueueFamilyIndex(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

			static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
				const PFN_vkCreateDevice pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(proc::in::getProcAddress(hVulkan, "vkCreateDevice"));

				if (!pVkCreateDevice) return VK_NULL_HANDLE;
			
				const uint32_t graphicsQueueFamilyIndex = getGraphicsQueueFamilyIndex(hVulkan, hPhysicalDevice);

				if (graphicsQueueFamilyIndex == UINT32_MAX) return VK_NULL_HANDLE;

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

				if (!pVkGetPhysicalDeviceQueueFamilyProperties) return UINT32_MAX;
			
				uint32_t tmpPropertiesCount = 0u;
				pVkGetPhysicalDeviceQueueFamilyProperties(hPhysicalDevice, &tmpPropertiesCount, nullptr);

				if (!tmpPropertiesCount) return UINT32_MAX;

				const uint32_t propertiesCount = tmpPropertiesCount;
				VkQueueFamilyProperties* const pProperties = new VkQueueFamilyProperties[propertiesCount]{};

				pVkGetPhysicalDeviceQueueFamilyProperties(hPhysicalDevice, &tmpPropertiesCount, pProperties);

				uint32_t queueFamily = UINT32_MAX;

				for (uint32_t i = 0u; i < propertiesCount; i++) {

					if (pProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
						queueFamily = i;
						break;
					}

				}

				delete[] pProperties;

				return queueFamily;
			}


			Backend::Backend() :
				_phPresentInfo{}, _hDevice{}, _hVulkan {}, _hMainWindow{}, _f{},
				_graphicsQueueFamilyIndex{ UINT32_MAX }, _memoryProperties{}, _hRenderPass{}, _hCommandPool{},
				_hDescriptorSetLayout{}, _hPipelineLayout{}, _hTriangleListPipeline{}, _hPointListPipeline{},
				_hFirstGraphicsQueue{}, _viewport{}, _pImageDataArray{}, _imageCount{}, _pCurImageData{} {}


			Backend::~Backend() {

				if (this->_hDevice == VK_NULL_HANDLE) return;

				if (
					this->_f.pVkFreeCommandBuffers && this->_f.pVkDestroyFramebuffer && this->_f.pVkDestroyImageView &&
					this->_f.pVkWaitForFences && this->_f.pVkDestroyFence &&
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

				if (this->_f.pVkDestroyCommandPool && this->_hCommandPool != VK_NULL_HANDLE) {
					this->_f.pVkDestroyCommandPool(this->_hDevice, this->_hCommandPool, nullptr);
				}

				if (this->_f.pVkDestroyRenderPass && this->_hRenderPass != VK_NULL_HANDLE) {
					this->_f.pVkDestroyRenderPass(this->_hDevice, this->_hRenderPass, nullptr);
				}

			}


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
				this->_phPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pArg1);
				this->_hDevice = reinterpret_cast<VkDevice>(pArg2);

				return;
			}


			static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

			bool Backend::initialize() {
				this->_hVulkan = proc::in::getModuleHandle("vulkan-1.dll");

				if (!this->_hVulkan) return false;

				EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&this->_hMainWindow));

				if (!this->_hMainWindow) return false;

				if (!this->getProcAddresses()) return false;

				if (!this->getPhysicalDeviceProperties()) return false;

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

				this->_f.pVkGetDeviceQueue(this->_hDevice, this->_graphicsQueueFamilyIndex, 0u, &this->_hFirstGraphicsQueue);

				if (this->_hFirstGraphicsQueue == VK_NULL_HANDLE) return false;

				return true;
			}


			bool Backend::beginFrame() {
				uint32_t imageCount{};

				if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, this->_phPresentInfo->pSwapchains[0], &imageCount, nullptr) != VkResult::VK_SUCCESS) return false;

				if (!imageCount) return false;

				if (this->_imageCount != imageCount) {
					this->destroyImageDataArray();

					if (!this->createImageDataArray(imageCount)) {
						this->destroyImageDataArray();

						return false;
					}

				}

				VkViewport viewport{};

				if (!this->getCurrentViewport(&viewport)) return false;

				if (this->_viewport.width != viewport.width || this->_viewport.height != viewport.height) {
					this->destroyFramebuffers();

					if (!this->createFramebuffers(viewport)) {
						this->destroyFramebuffers();

						return false;
					}

				}

				this->_pCurImageData = &this->_pImageDataArray[this->_phPresentInfo->pImageIndices[0]];

				this->_f.pVkWaitForFences(this->_hDevice, 1u, &this->_pCurImageData->hFence, VK_TRUE, UINT64_MAX);
				this->_f.pVkResetFences(this->_hDevice, 1u, &this->_pCurImageData->hFence);

				if (!this->beginCommandBuffer(this->_pCurImageData->hCommandBuffer)) return false;

				this->beginRenderPass(this->_pCurImageData->hCommandBuffer, this->_pCurImageData->hFrameBuffer);

				this->_f.pVkCmdSetViewport(this->_pCurImageData->hCommandBuffer, 0u, 1u, &this->_viewport);

				const VkRect2D scissor{ { static_cast<int32_t>(this->_viewport.x), static_cast<int32_t>(this->_viewport.y) }, { static_cast<uint32_t>(this->_viewport.width), static_cast<uint32_t>(this->_viewport.height) } };
				this->_f.pVkCmdSetScissor(this->_pCurImageData->hCommandBuffer, 0u, 1u, &scissor);

				const float scale[]{ 2.f / this->_viewport.width, 2.f / this->_viewport.height };
				this->_f.pVkCmdPushConstants(this->_pCurImageData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(scale), scale);

				const float translate[]{ -1.f, -1.f };
				this->_f.pVkCmdPushConstants(this->_pCurImageData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(scale), sizeof(translate), translate);

				return true;
			}


			void Backend::endFrame() {
				this->_f.pVkCmdEndRenderPass(this->_pCurImageData->hCommandBuffer);
				this->_f.pVkEndCommandBuffer(this->_pCurImageData->hCommandBuffer);

				VkPipelineStageFlags* const pStageMask = new VkPipelineStageFlags[this->_phPresentInfo->waitSemaphoreCount];
				
				for (uint32_t i = 0u; i < this->_phPresentInfo->waitSemaphoreCount; i++) {
					pStageMask[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
			
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.pWaitDstStageMask = pStageMask;
				submitInfo.commandBufferCount = 1u;
				submitInfo.pCommandBuffers = &this->_pCurImageData->hCommandBuffer;
				submitInfo.pWaitSemaphores = this->_phPresentInfo->pWaitSemaphores;
				submitInfo.waitSemaphoreCount = this->_phPresentInfo->waitSemaphoreCount;
				submitInfo.pSignalSemaphores = this->_phPresentInfo->pWaitSemaphores;
				submitInfo.signalSemaphoreCount = this->_phPresentInfo->waitSemaphoreCount;

				this->_f.pVkQueueSubmit(this->_hFirstGraphicsQueue, 1u, &submitInfo, this->_pCurImageData->hFence);

				delete[] pStageMask;

				return;
			}


			AbstractDrawBuffer* Backend::getTriangleListBuffer() {

				return &this->_pCurImageData->triangleListBuffer;
			}


			AbstractDrawBuffer* Backend::getPointListBuffer() {

				return &this->_pCurImageData->pointListBuffer;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = this->_viewport.width;
				*frameHeight = this->_viewport.height;

				return;
			}


			#define ASSIGN_DEVICE_PROC_ADDRESS(f) this->_f.pVk##f = reinterpret_cast<PFN_vk##f>(pVkGetDeviceProcAddress(this->_hDevice, "vk"#f))

			bool Backend::getProcAddresses() {
				const PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddress = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetDeviceProcAddr"));
			
				if (!pVkGetDeviceProcAddress) return false;
				
				ASSIGN_DEVICE_PROC_ADDRESS(GetSwapchainImagesKHR);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateCommandPool);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyCommandPool);
				ASSIGN_DEVICE_PROC_ADDRESS(AllocateCommandBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(FreeCommandBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateShaderModule);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyShaderModule);
				ASSIGN_DEVICE_PROC_ADDRESS(CreatePipelineLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyPipelineLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateDescriptorSetLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyDescriptorSetLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateGraphicsPipelines);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyPipeline);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(GetBufferMemoryRequirements);
				ASSIGN_DEVICE_PROC_ADDRESS(AllocateMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(BindBufferMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(FreeMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateImageView);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyImageView);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateFramebuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyFramebuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateFence);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyFence);
				ASSIGN_DEVICE_PROC_ADDRESS(WaitForFences);
				ASSIGN_DEVICE_PROC_ADDRESS(ResetFences);
				ASSIGN_DEVICE_PROC_ADDRESS(ResetCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(BeginCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(EndCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBeginRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdEndRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(MapMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(UnmapMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(FlushMappedMemoryRanges);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindPipeline);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindVertexBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindIndexBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdSetViewport);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdPushConstants);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdSetScissor);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdDrawIndexed);
				ASSIGN_DEVICE_PROC_ADDRESS(GetDeviceQueue);
				ASSIGN_DEVICE_PROC_ADDRESS(QueueSubmit);

				for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {
				
					if (!(this->_fPtrs[i])) return false;

				}

				return true;
			}

			#undef ASSIGN_PROC_ADDRESS

			
			bool Backend::getPhysicalDeviceProperties() {
				const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(this->_hVulkan, "vkDestroyInstance"));
				const PFN_vkGetInstanceProcAddr pVkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetInstanceProcAddr"));
				
				if (!pVkDestroyInstance || !pVkGetInstanceProcAddr) return false;

				const VkInstance hInstance = createInstance(this->_hVulkan);

				if (hInstance == VK_NULL_HANDLE) return false;

				const PFN_vkGetPhysicalDeviceMemoryProperties pVkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(pVkGetInstanceProcAddr(hInstance, "vkGetPhysicalDeviceMemoryProperties"));

				if (!pVkGetPhysicalDeviceMemoryProperties) {
					pVkDestroyInstance(hInstance, nullptr);

					return false;
				}

				const VkPhysicalDevice hPhysicalDevice = getPhysicalDevice(this->_hVulkan, hInstance);

				if (hPhysicalDevice == VK_NULL_HANDLE) {
					pVkDestroyInstance(hInstance, nullptr);

					return false;
				}

				this->_graphicsQueueFamilyIndex = getGraphicsQueueFamilyIndex(this->_hVulkan, hPhysicalDevice);

				if (this->_graphicsQueueFamilyIndex == UINT32_MAX) {
					pVkDestroyInstance(hInstance, nullptr);

					return false;
				}

				pVkGetPhysicalDeviceMemoryProperties(hPhysicalDevice, &this->_memoryProperties);

				if (!this->_memoryProperties.memoryTypeCount) {
					pVkDestroyInstance(hInstance, nullptr);

					return false;
				}

				pVkDestroyInstance(hInstance, nullptr);

				return true;
			}


			bool Backend::createRenderPass() {
				VkAttachmentDescription attachmentDesc{};
				attachmentDesc.format = VK_FORMAT_B8G8R8A8_UNORM;
				attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
				attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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


			bool Backend::createCommandPool() {
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
			layout(location = 0) out vec4 fragColor;

			void main()
			{
				gl_PointSize = 1.0f;
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

			VkPipeline Backend::createPipeline(VkPrimitiveTopology topology) {

				if (this->_hPipelineLayout == VK_NULL_HANDLE) {

					if (!this->createPipelineLayout()) return VK_NULL_HANDLE;

				}

				const VkShaderModule hShaderModuleVert = this->createShaderModule(GLSL_SHADER_VERT, sizeof(GLSL_SHADER_VERT));

				if (hShaderModuleVert == VK_NULL_HANDLE) return VK_NULL_HANDLE;

				const VkShaderModule hShaderModuleFrag = this->createShaderModule(GLSL_SHADER_FRAG, sizeof(GLSL_SHADER_FRAG));
				
				if (hShaderModuleFrag == VK_NULL_HANDLE) {
					this->_f.pVkDestroyShaderModule(this->_hDevice, hShaderModuleVert, nullptr);

					return VK_NULL_HANDLE;
				}

				VkPipelineShaderStageCreateInfo stageCreateInfo[2]{};
				stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
				stageCreateInfo[0].module = hShaderModuleVert;
				stageCreateInfo[0].pName = "main";
				stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				stageCreateInfo[1].module = hShaderModuleFrag;
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

				this->_f.pVkDestroyShaderModule(this->_hDevice, hShaderModuleFrag, nullptr);
				this->_f.pVkDestroyShaderModule(this->_hDevice, hShaderModuleVert, nullptr);

				return hPipeline;
			}


			VkShaderModule Backend::createShaderModule(const BYTE shader[], size_t size) const {
				VkShaderModuleCreateInfo fragCreateInfo{};
				fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				fragCreateInfo.codeSize = size;
				fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shader);

				VkShaderModule hShaderModule{};

				if (this->_f.pVkCreateShaderModule(this->_hDevice, &fragCreateInfo, nullptr, &hShaderModule) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

				return hShaderModule;
			}


			bool Backend::createPipelineLayout() {

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


			bool Backend::createDescriptorSetLayout() {
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


			bool Backend::createImageDataArray(uint32_t imageCount) {
				this->_pImageDataArray = new ImageData[imageCount]{};
				this->_imageCount = imageCount;
			
				for (uint32_t i = 0; i < this->_imageCount; i++) {
					VkCommandBufferAllocateInfo commandBufferAllocInfo{};
					commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferAllocInfo.commandPool = this->_hCommandPool;
					commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferAllocInfo.commandBufferCount = 1u;
						
					if (this->_f.pVkAllocateCommandBuffers(this->_hDevice, &commandBufferAllocInfo, &this->_pImageDataArray[i].hCommandBuffer) != VkResult::VK_SUCCESS) return false;

					this->_pImageDataArray[i].triangleListBuffer.initialize(this->_f, this->_hDevice, this->_pImageDataArray[i].hCommandBuffer, this->_hTriangleListPipeline, this->_memoryProperties);

					static constexpr size_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

					if (!this->_pImageDataArray[i].triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

					this->_pImageDataArray[i].pointListBuffer.initialize(this->_f, this->_hDevice, this->_pImageDataArray[i].hCommandBuffer, this->_hPointListPipeline, this->_memoryProperties);

					static constexpr size_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;

					if (!this->_pImageDataArray[i].pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) return false;

					VkFenceCreateInfo fenceCreateInfo{};
					fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
					fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

					if (this->_f.pVkCreateFence(this->_hDevice, &fenceCreateInfo, nullptr, &this->_pImageDataArray[i].hFence) != VkResult::VK_SUCCESS) return false;

				}

				return true;
			}


			void Backend::destroyImageDataArray() {

				if (!this->_pImageDataArray) return;

				for (uint32_t i = 0u; i < this->_imageCount && this->_hDevice != VK_NULL_HANDLE; i++) {
					this->destroyImageData(&this->_pImageDataArray[i]);
				}

				delete[] this->_pImageDataArray;
				this->_pImageDataArray = nullptr;
				this->_imageCount = 0u;

				return;
			}


			void Backend::destroyImageData(ImageData* pImageData) const {

				if (pImageData->hFence != VK_NULL_HANDLE) {
					this->_f.pVkWaitForFences(this->_hDevice, 1u, &pImageData->hFence, VK_TRUE, UINT64_MAX);
					this->_f.pVkDestroyFence(this->_hDevice, pImageData->hFence, nullptr);
					pImageData->hFence = VK_NULL_HANDLE;
				}

				if (pImageData->hFrameBuffer != VK_NULL_HANDLE) {
					this->_f.pVkDestroyFramebuffer(this->_hDevice, pImageData->hFrameBuffer, nullptr);
					pImageData->hFrameBuffer = VK_NULL_HANDLE;
				}

				if (pImageData->hImageView != VK_NULL_HANDLE) {
					this->_f.pVkDestroyImageView(this->_hDevice, pImageData->hImageView, nullptr);
					pImageData->hImageView = VK_NULL_HANDLE;
				}

				pImageData->pointListBuffer.destroy();
				pImageData->triangleListBuffer.destroy();

				if (pImageData->hCommandBuffer != VK_NULL_HANDLE) {
					this->_f.pVkFreeCommandBuffers(this->_hDevice, this->_hCommandPool, 1u, &pImageData->hCommandBuffer);
					pImageData->hCommandBuffer = VK_NULL_HANDLE;
				}

				return;
			}


			bool Backend::createFramebuffers(VkViewport viewport) {
				VkImage* const pImages = new VkImage[this->_imageCount]{};

				uint32_t imageCount = this->_imageCount;

				if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, this->_phPresentInfo->pSwapchains[0], &imageCount, pImages) != VkResult::VK_SUCCESS || imageCount != this->_imageCount) {
					delete[] pImages;

					return false;
				}

				for (uint32_t i = 0; i < this->_imageCount; i++) {
					VkImageSubresourceRange imageSubresourceRange{};
					imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageSubresourceRange.baseMipLevel = 0u;
					imageSubresourceRange.levelCount = 1u;
					imageSubresourceRange.baseArrayLayer = 0u;
					imageSubresourceRange.layerCount = 1u;

					VkImageViewCreateInfo imageViewCreateInfo{};
					imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					imageViewCreateInfo.image = pImages[i];
					imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
					imageViewCreateInfo.subresourceRange = imageSubresourceRange;
					
					if (this->_f.pVkCreateImageView(this->_hDevice, &imageViewCreateInfo, nullptr, &this->_pImageDataArray[i].hImageView) != VkResult::VK_SUCCESS) {
						delete[] pImages;
					
						return false;
					}

					VkFramebufferCreateInfo framebufferCreateInfo{};
					framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					framebufferCreateInfo.renderPass = this->_hRenderPass;
					framebufferCreateInfo.attachmentCount = 1u;
					framebufferCreateInfo.pAttachments = &this->_pImageDataArray[i].hImageView;
					framebufferCreateInfo.layers = 1u;
					framebufferCreateInfo.width = static_cast<uint32_t>(viewport.width);
					framebufferCreateInfo.height = static_cast<uint32_t>(viewport.height);

					if (this->_f.pVkCreateFramebuffer(this->_hDevice, &framebufferCreateInfo, nullptr, &this->_pImageDataArray[i].hFrameBuffer) != VkResult::VK_SUCCESS) {
						delete[] pImages;

						return false;
					}

				}

				delete[] pImages;
				
				this->_viewport = viewport;

				return true;
			}


			void Backend::destroyFramebuffers() {
			
				for (uint32_t i = 0u; i < this->_imageCount; i++) {

					if (this->_pImageDataArray[i].hFence != VK_NULL_HANDLE) {
						this->_f.pVkWaitForFences(this->_hDevice, 1u, &this->_pImageDataArray[i].hFence, VK_TRUE, UINT64_MAX);
					}
				
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


			bool Backend::getCurrentViewport(VkViewport* pViewport) const {
				RECT clientRect{};

				if (!GetClientRect(this->_hMainWindow, &clientRect)) return false;

				pViewport->x = static_cast<float>(clientRect.left);
				pViewport->y = static_cast<float>(clientRect.top);
				pViewport->width = static_cast<float>(clientRect.right - clientRect.left);
				pViewport->height = static_cast<float>(clientRect.bottom - clientRect.top);
				pViewport->minDepth = 0.f;
				pViewport->maxDepth = 1.f;

				return true;
			}
			
			
			bool Backend::beginCommandBuffer(VkCommandBuffer hCommandBuffer) const {
			
				if (this->_f.pVkResetCommandBuffer(hCommandBuffer, 0u) != VkResult::VK_SUCCESS) return false;

				VkCommandBufferBeginInfo cmdBufferBeginInfo{};
				cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				return this->_f.pVkBeginCommandBuffer(hCommandBuffer, &cmdBufferBeginInfo) == VkResult::VK_SUCCESS;
			}


			void Backend::beginRenderPass(VkCommandBuffer hCommandBuffer, VkFramebuffer hFramebuffer) const {
				VkRenderPassBeginInfo renderPassBeginInfo{};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = this->_hRenderPass;
				renderPassBeginInfo.framebuffer = hFramebuffer;
				renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(this->_viewport.width);
				renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(this->_viewport.height);
				this->_f.pVkCmdBeginRenderPass(hCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

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

}