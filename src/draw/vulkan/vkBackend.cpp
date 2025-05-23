#include "vkBackend.h"
#include "vkShaders.h"
#include "..\..\hooks\TrampHook.h"
#include "..\..\proc.h"

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

				if (pVkCreateInstance(&createInfo, nullptr, &hInstance) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hInstance;
			}


			static VkPhysicalDevice getPhysicalDevice(HMODULE hVulkan, VkInstance hInstance) {
				const PFN_vkEnumeratePhysicalDevices pVkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(hax::proc::in::getProcAddress(hVulkan, "vkEnumeratePhysicalDevices"));
				const PFN_vkGetPhysicalDeviceProperties pVkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(hax::proc::in::getProcAddress(hVulkan, "vkGetPhysicalDeviceProperties"));
			
				if (!pVkEnumeratePhysicalDevices || !pVkGetPhysicalDeviceProperties) return VK_NULL_HANDLE;
			
				uint32_t tmpGpuCount = 0u;

				if (pVkEnumeratePhysicalDevices(hInstance, &tmpGpuCount, nullptr) != VK_SUCCESS) return VK_NULL_HANDLE;

				if (!tmpGpuCount) return VK_NULL_HANDLE;

				const uint32_t gpuCount = tmpGpuCount;
				VkPhysicalDevice* const pPhysicalDeviceArray = new VkPhysicalDevice[gpuCount]{};

				if (pVkEnumeratePhysicalDevices(hInstance, &tmpGpuCount, pPhysicalDeviceArray) != VK_SUCCESS) return VK_NULL_HANDLE;

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

				if (pVkCreateDevice(hPhysicalDevice, &deviceCreateInfo, nullptr, &hDummyDevice) != VK_SUCCESS) return VK_NULL_HANDLE;

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
				_phPresentInfo{}, _hDevice{}, _hVulkan {}, _hMainWindow{}, _f{}, _hRenderPass{}, _graphicsQueueFamilyIndex{ UINT32_MAX }, _memoryProperties{},
				_hCommandPool{}, _hTextureCommandBuffer{}, _hTextureSampler{}, _hDescriptorPool{}, _hDescriptorSetLayout{},
				_hPipelineLayout{}, _hPipelineTexture{}, _hPipelinePassthrough{}, _hFirstGraphicsQueue{}, _viewport{}, _frameDataVector{}, _pCurFrameData{} {}


			Backend::~Backend() {

				if (this->_hDevice == VK_NULL_HANDLE) return;

				this->_frameDataVector.resize(0u);

				if (this->_f.pVkDestroyPipeline) {

					if (this->_hPipelinePassthrough != VK_NULL_HANDLE) {
						this->_f.pVkDestroyPipeline(this->_hDevice, this->_hPipelinePassthrough, nullptr);
					}

					if (this->_hPipelineTexture != VK_NULL_HANDLE) {
						this->_f.pVkDestroyPipeline(this->_hDevice, this->_hPipelineTexture, nullptr);
					}

				}

				if (this->_f.pVkDestroyPipelineLayout && this->_hPipelineLayout != VK_NULL_HANDLE) {
					this->_f.pVkDestroyPipelineLayout(this->_hDevice, this->_hPipelineLayout, nullptr);
				}

				if (this->_f.pVkDestroyDescriptorSetLayout && this->_hDescriptorSetLayout != VK_NULL_HANDLE) {
					this->_f.pVkDestroyDescriptorSetLayout(this->_hDevice, this->_hDescriptorSetLayout, nullptr);
				}

				if (this->_f.pVkDestroyDescriptorPool && this->_hDescriptorPool != VK_NULL_HANDLE) {
					this->_f.pVkDestroyDescriptorPool(this->_hDevice, this->_hDescriptorPool, nullptr);
				}

				if (this->_f.pVkDestroySampler && this->_hTextureSampler != VK_NULL_HANDLE) {
					this->_f.pVkDestroySampler(this->_hDevice, this->_hTextureSampler, nullptr);
				}

				if (this->_f.pVkFreeCommandBuffers && this->_hTextureCommandBuffer != VK_NULL_HANDLE) {
					this->_f.pVkFreeCommandBuffers(this->_hDevice, this->_hCommandPool, 1u, &this->_hTextureCommandBuffer);
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

				if (this->_hTextureCommandBuffer == VK_NULL_HANDLE) {
					this->_hTextureCommandBuffer = this->allocCommandBuffer();
				}

				if (this->_hTextureCommandBuffer == VK_NULL_HANDLE) return false;

				if (this->_hTextureSampler == VK_NULL_HANDLE) {

					if (!this->createTextureSampler()) return false;

				}
				
				if (this->_hDescriptorPool == VK_NULL_HANDLE) {

					if (!this->createDescriptorPool()) return false;

				}

				if (this->_hDescriptorSetLayout == VK_NULL_HANDLE) {

					if (!this->createDescriptorSetLayout()) return false;

				}

				if (this->_hPipelineLayout == VK_NULL_HANDLE) {

					if (!this->createPipelineLayout()) return VK_NULL_HANDLE;

				}

				if (this->_hPipelineTexture == VK_NULL_HANDLE) {
					this->_hPipelineTexture = this->createPipeline(FRAGMENT_SHADER_TEXTURE, sizeof(FRAGMENT_SHADER_TEXTURE));
				}

				if (this->_hPipelineTexture == VK_NULL_HANDLE) return false;

				if (this->_hPipelinePassthrough == VK_NULL_HANDLE) {
					this->_hPipelinePassthrough = this->createPipeline(FRAGMENT_SHADER_PASSTHROUGH, sizeof(FRAGMENT_SHADER_PASSTHROUGH));
				}

				if (this->_hPipelinePassthrough == VK_NULL_HANDLE) return false;

				this->_f.pVkGetDeviceQueue(this->_hDevice, this->_graphicsQueueFamilyIndex, 0u, &this->_hFirstGraphicsQueue);

				if (this->_hFirstGraphicsQueue == VK_NULL_HANDLE) return false;

				return true;
			}


			TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				TextureData textureData{};

				textureData.hImage = this->createImage(width, height);
				
				if (textureData.hImage == VK_NULL_HANDLE) {
					
					return 0ull;
				}

				VkMemoryRequirements memRequirementsImage{};
				this->_f.pVkGetImageMemoryRequirements(this->_hDevice, textureData.hImage, &memRequirementsImage);

				textureData.hMemory = this->allocateMemory(&memRequirementsImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				
				if (textureData.hMemory == VK_NULL_HANDLE) {
					this->destroyTextureData(&textureData);
					
					return 0ull;
				}

				if (this->_f.pVkBindImageMemory(this->_hDevice, textureData.hImage, textureData.hMemory, 0ull) != VK_SUCCESS) {
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				textureData.hImageView = this->createImageView(textureData.hImage);

				if (textureData.hImageView == VK_NULL_HANDLE) {
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				textureData.hDescriptorSet = this->createDescriptorSet(textureData.hImageView);

				if (textureData.hDescriptorSet == VK_NULL_HANDLE) {
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				const VkDeviceSize size = width * height * sizeof(Color);
				const VkBuffer hUploadBuffer = this->createBuffer(size);

				if (hUploadBuffer == VK_NULL_HANDLE) {
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				VkMemoryRequirements memRequirementsBuffer{};
				this->_f.pVkGetBufferMemoryRequirements(this->_hDevice, hUploadBuffer, &memRequirementsBuffer);
				const VkDeviceMemory hUploadMemory = this->allocateMemory(&memRequirementsBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

				if (hUploadMemory == VK_NULL_HANDLE) {
					this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				if (this->_f.pVkBindBufferMemory(this->_hDevice, hUploadBuffer, hUploadMemory, 0ull) != VK_SUCCESS) {
					this->_f.pVkFreeMemory(this->_hDevice, hUploadMemory, nullptr);
					this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				Color* pLocalBuffer = nullptr;
				
				if (this->_f.pVkMapMemory(this->_hDevice, hUploadMemory, 0ull, size, 0u, reinterpret_cast<void**>(&pLocalBuffer)) != VK_SUCCESS) {
					this->_f.pVkFreeMemory(this->_hDevice, hUploadMemory, nullptr);
					this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				memcpy(pLocalBuffer, data, static_cast<size_t>(size));
				
				VkMappedMemoryRange range{};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.memory = hUploadMemory;
				range.size = size;

				if (this->_f.pVkFlushMappedMemoryRanges(this->_hDevice, 1u, &range) != VK_SUCCESS) {
					this->_f.pVkFreeMemory(this->_hDevice, hUploadMemory, nullptr);
					this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				this->_f.pVkUnmapMemory(this->_hDevice, hUploadMemory);

				if (!this->uploadImage(textureData.hImage, hUploadBuffer, width, height)) {
					this->_f.pVkFreeMemory(this->_hDevice, hUploadMemory, nullptr);
					this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);
					this->destroyTextureData(&textureData);

					return 0ull;
				}

				this->_f.pVkFreeMemory(this->_hDevice, hUploadMemory, nullptr);
				this->_f.pVkDestroyBuffer(this->_hDevice, hUploadBuffer, nullptr);

				#ifdef _WIN64

				return reinterpret_cast<TextureId>(textureData.hDescriptorSet);

				#else

				return textureData.hDescriptorSet;

				#endif
			}


			bool Backend::beginFrame() {
				uint32_t imageCount = 0u;

				if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, this->_phPresentInfo->pSwapchains[0], &imageCount, nullptr) != VK_SUCCESS) return false;

				if (!imageCount) return false;

				VkViewport viewport{};

				if (!this->getCurrentViewport(&viewport)) return false;

				if (this->_frameDataVector.size() != imageCount || this->_viewport.width != viewport.width || this->_viewport.height != viewport.height) {

					if (!this->resizeFrameDataVector(imageCount, viewport)) return false;

					this->_viewport = viewport;
				}

				this->_pCurFrameData = this->_frameDataVector + this->_phPresentInfo->pImageIndices[0];

				this->_f.pVkWaitForFences(this->_hDevice, 1u, &this->_pCurFrameData->hFence, VK_TRUE, UINT64_MAX);
				this->_f.pVkResetFences(this->_hDevice, 1u, &this->_pCurFrameData->hFence);

				if (!this->beginCommandBuffer(this->_pCurFrameData->hCommandBuffer)) return false;

				this->beginRenderPass(this->_pCurFrameData->hCommandBuffer, this->_pCurFrameData->hFrameBuffer);

				this->_f.pVkCmdSetViewport(this->_pCurFrameData->hCommandBuffer, 0u, 1u, &this->_viewport);

				const VkRect2D scissor{ { static_cast<int32_t>(this->_viewport.x), static_cast<int32_t>(this->_viewport.y) }, { static_cast<uint32_t>(this->_viewport.width), static_cast<uint32_t>(this->_viewport.height) } };
				this->_f.pVkCmdSetScissor(this->_pCurFrameData->hCommandBuffer, 0u, 1u, &scissor);

				const float scale[]{ 2.f / this->_viewport.width, 2.f / this->_viewport.height };
				this->_f.pVkCmdPushConstants(this->_pCurFrameData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(scale), scale);

				const float translate[]{ -1.f, -1.f };
				this->_f.pVkCmdPushConstants(this->_pCurFrameData->hCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(scale), sizeof(translate), translate);

				return true;
			}


			void Backend::endFrame() {
				this->_f.pVkCmdEndRenderPass(this->_pCurFrameData->hCommandBuffer);
				this->_f.pVkEndCommandBuffer(this->_pCurFrameData->hCommandBuffer);

				VkPipelineStageFlags* const pStageMask = new VkPipelineStageFlags[this->_phPresentInfo->waitSemaphoreCount];
				
				for (uint32_t i = 0u; i < this->_phPresentInfo->waitSemaphoreCount; i++) {
					pStageMask[i] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
			
				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.pWaitDstStageMask = pStageMask;
				submitInfo.commandBufferCount = 1u;
				submitInfo.pCommandBuffers = &this->_pCurFrameData->hCommandBuffer;
				submitInfo.pWaitSemaphores = this->_phPresentInfo->pWaitSemaphores;
				submitInfo.waitSemaphoreCount = this->_phPresentInfo->waitSemaphoreCount;
				submitInfo.pSignalSemaphores = this->_phPresentInfo->pWaitSemaphores;
				submitInfo.signalSemaphoreCount = this->_phPresentInfo->waitSemaphoreCount;

				this->_f.pVkQueueSubmit(this->_hFirstGraphicsQueue, 1u, &submitInfo, this->_pCurFrameData->hFence);

				delete[] pStageMask;

				return;
			}


			IBufferBackend* Backend::getTextureBufferBackend() {

				return &this->_pCurFrameData->textureBufferBackend;
			}


			IBufferBackend* Backend::getSolidBufferBackend() {

				return &this->_pCurFrameData->solidBufferBackend;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = this->_viewport.width;
				*frameHeight = this->_viewport.height;

				return;
			}


			#define ASSIGN_DEVICE_PROC_ADDRESS(f) this->_f.pVk##f = reinterpret_cast<PFN_vk##f>(pVkGetDeviceProcAddr(this->_hDevice, "vk"#f))

			bool Backend::getProcAddresses() {
				const PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(this->_hVulkan, "vkGetDeviceProcAddr"));
			
				if (!pVkGetDeviceProcAddr) return false;
				
				ASSIGN_DEVICE_PROC_ADDRESS(CreateCommandPool);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyCommandPool);
				ASSIGN_DEVICE_PROC_ADDRESS(AllocateCommandBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(FreeCommandBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateSampler);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroySampler);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateDescriptorPool);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyDescriptorPool);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(CreatePipelineLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyPipelineLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateDescriptorSetLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyDescriptorSetLayout);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateShaderModule);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyShaderModule);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateGraphicsPipelines);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyPipeline);
				ASSIGN_DEVICE_PROC_ADDRESS(GetDeviceQueue);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateImage);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyImage);
				ASSIGN_DEVICE_PROC_ADDRESS(GetImageMemoryRequirements);
				ASSIGN_DEVICE_PROC_ADDRESS(AllocateMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(FreeMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(BindImageMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateImageView);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyImageView);
				ASSIGN_DEVICE_PROC_ADDRESS(AllocateDescriptorSets);
				ASSIGN_DEVICE_PROC_ADDRESS(UpdateDescriptorSets);
				ASSIGN_DEVICE_PROC_ADDRESS(FreeDescriptorSets);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(GetBufferMemoryRequirements);
				ASSIGN_DEVICE_PROC_ADDRESS(BindBufferMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(MapMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(UnmapMemory);
				ASSIGN_DEVICE_PROC_ADDRESS(FlushMappedMemoryRanges);
				ASSIGN_DEVICE_PROC_ADDRESS(ResetCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(BeginCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(EndCommandBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdPipelineBarrier);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdCopyBufferToImage);
				ASSIGN_DEVICE_PROC_ADDRESS(QueueSubmit);
				ASSIGN_DEVICE_PROC_ADDRESS(QueueWaitIdle);
				ASSIGN_DEVICE_PROC_ADDRESS(GetSwapchainImagesKHR);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateFence);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyFence);
				ASSIGN_DEVICE_PROC_ADDRESS(WaitForFences);
				ASSIGN_DEVICE_PROC_ADDRESS(ResetFences);
				ASSIGN_DEVICE_PROC_ADDRESS(CreateFramebuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(DestroyFramebuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBeginRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdEndRenderPass);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdSetScissor);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdSetViewport);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdPushConstants);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindPipeline);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindVertexBuffers);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindIndexBuffer);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdBindDescriptorSets);
				ASSIGN_DEVICE_PROC_ADDRESS(CmdDrawIndexed);

				for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {
				
					if (!(this->_fPtrs[i])) return false;

				}

				return true;
			}

			#undef ASSIGN_PROC_ADDRESS


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

				return this->_f.pVkCreateRenderPass(this->_hDevice, &renderPassCreateInfo, nullptr, &this->_hRenderPass) == VK_SUCCESS;
			}

			
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
			
			
			bool Backend::createCommandPool() {
				VkCommandPoolCreateInfo commandPoolCreateInfo{};
				commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
				commandPoolCreateInfo.queueFamilyIndex = this->_graphicsQueueFamilyIndex;

				return this->_f.pVkCreateCommandPool(this->_hDevice, &commandPoolCreateInfo, nullptr, &this->_hCommandPool) == VK_SUCCESS;
			}


			VkCommandBuffer Backend::allocCommandBuffer() const {
				VkCommandBufferAllocateInfo commandBufferAllocInfo{};
				commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferAllocInfo.commandPool = this->_hCommandPool;
				commandBufferAllocInfo.commandBufferCount = 1u;

				VkCommandBuffer hCommandBuffer = VK_NULL_HANDLE;

				if (this->_f.pVkAllocateCommandBuffers(this->_hDevice, &commandBufferAllocInfo, &hCommandBuffer) != VK_SUCCESS) {
					
					return VK_NULL_HANDLE;
				}

				return hCommandBuffer;
			}


			bool Backend::createTextureSampler() {
				VkSamplerCreateInfo samplerInfo{};
				samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerInfo.magFilter = VK_FILTER_LINEAR;
				samplerInfo.minFilter = VK_FILTER_LINEAR;

				return this->_f.pVkCreateSampler(this->_hDevice, &samplerInfo, nullptr, &this->_hTextureSampler) == VK_SUCCESS;
			}


			bool Backend::createDescriptorPool() {
				// 1000 textures should be enough, otherwise bad luck
				constexpr uint32_t descPoolSize = 1000u;
				VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descPoolSize };
				
				VkDescriptorPoolCreateInfo poolCreateInfo{};
				poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				poolCreateInfo.maxSets = descPoolSize;
				poolCreateInfo.poolSizeCount = 1u;
				poolCreateInfo.pPoolSizes = &poolSize;

				return this->_f.pVkCreateDescriptorPool(this->_hDevice, &poolCreateInfo, nullptr, &this->_hDescriptorPool) == VK_SUCCESS;
			}


			bool Backend::createDescriptorSetLayout() {
				VkDescriptorSetLayoutBinding binding{};
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.descriptorCount = 1u;
				binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				VkDescriptorSetLayoutCreateInfo descCreateinfo{};
				descCreateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				descCreateinfo.bindingCount = 1u;
				descCreateinfo.pBindings = &binding;

				return this->_f.pVkCreateDescriptorSetLayout(this->_hDevice, &descCreateinfo, nullptr, &this->_hDescriptorSetLayout) == VK_SUCCESS;
			}


			bool Backend::createPipelineLayout() {
				VkPushConstantRange pushConstants{};
				pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				pushConstants.size = sizeof(float) * 4u;

				VkPipelineLayoutCreateInfo layoutCreateInfo{};
				layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				layoutCreateInfo.setLayoutCount = 1u;
				layoutCreateInfo.pSetLayouts = &this->_hDescriptorSetLayout;
				layoutCreateInfo.pushConstantRangeCount = 1u;
				layoutCreateInfo.pPushConstantRanges = &pushConstants;

				return this->_f.pVkCreatePipelineLayout(this->_hDevice, &layoutCreateInfo, nullptr, &this->_hPipelineLayout) == VK_SUCCESS;
			}


			VkPipeline Backend::createPipeline(const unsigned char* pFragmentShader, size_t fragmentShaderSize) const {
				const VkShaderModule hShaderModuleVert = this->createShaderModule(VERTEX_SHADER, sizeof(VERTEX_SHADER));

				if (hShaderModuleVert == VK_NULL_HANDLE) return VK_NULL_HANDLE;

				const VkShaderModule hShaderModuleFrag = this->createShaderModule(pFragmentShader, fragmentShaderSize);
				
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

				VkVertexInputAttributeDescription attributeDesc[3]{};
				attributeDesc[0].location = 0u;
				attributeDesc[0].binding = bindingDesc.binding;
				attributeDesc[0].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDesc[0].offset = 0u;
				attributeDesc[1].location = 1u;
				attributeDesc[1].binding = bindingDesc.binding;
				attributeDesc[1].format = VK_FORMAT_B8G8R8A8_UNORM;
				attributeDesc[1].offset = sizeof(Vector2);
				attributeDesc[2].location = 2u;
				attributeDesc[2].binding = bindingDesc.binding;
				attributeDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
				attributeDesc[2].offset = sizeof(Vector2) + sizeof(Color);

				VkPipelineVertexInputStateCreateInfo vertexInfo{};
				vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInfo.vertexBindingDescriptionCount = 1u;
				vertexInfo.pVertexBindingDescriptions = &bindingDesc;
				vertexInfo.vertexAttributeDescriptionCount = _countof(attributeDesc);
				vertexInfo.pVertexAttributeDescriptions = attributeDesc;

				VkPipelineInputAssemblyStateCreateInfo iaInfo{};
				iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				iaInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

				VkPipelineViewportStateCreateInfo viewportInfo{};
				viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportInfo.viewportCount = 1u;
				viewportInfo.scissorCount = 1u;

				VkPipelineRasterizationStateCreateInfo rasterInfo{};
				rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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

				VkPipeline hPipeline = VK_NULL_HANDLE;

				if (this->_f.pVkCreateGraphicsPipelines(this->_hDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &hPipeline) != VK_SUCCESS) return VK_NULL_HANDLE;

				this->_f.pVkDestroyShaderModule(this->_hDevice, hShaderModuleFrag, nullptr);
				this->_f.pVkDestroyShaderModule(this->_hDevice, hShaderModuleVert, nullptr);

				return hPipeline;
			}


			VkShaderModule Backend::createShaderModule(const unsigned char* pShader, size_t size) const {
				VkShaderModuleCreateInfo fragCreateInfo{};
				fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				fragCreateInfo.codeSize = size;
				fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(pShader);

				VkShaderModule hShaderModule = VK_NULL_HANDLE;

				if (this->_f.pVkCreateShaderModule(this->_hDevice, &fragCreateInfo, nullptr, &hShaderModule) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hShaderModule;
			}


			VkImage Backend::createImage(uint32_t width, uint32_t height) const {
				VkImageCreateInfo imageCreateInfo{};
				imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
				imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
				imageCreateInfo.extent.width = width;
				imageCreateInfo.extent.height = height;
				imageCreateInfo.extent.depth = 1u;
				imageCreateInfo.mipLevels = 1u;
				imageCreateInfo.arrayLayers = 1u;
				imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

				VkImage hImage = VK_NULL_HANDLE;

				if (this->_f.pVkCreateImage(this->_hDevice, &imageCreateInfo, nullptr, &hImage) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hImage;
			}


			void Backend::destroyTextureData(TextureData* pTextureData) const {

				if (pTextureData->hDescriptorSet != VK_NULL_HANDLE) {
					this->_f.pVkFreeDescriptorSets(this->_hDevice, this->_hDescriptorPool, 1u, &pTextureData->hDescriptorSet);
					pTextureData->hDescriptorSet = VK_NULL_HANDLE;
				}
				
				if (pTextureData->hImageView != VK_NULL_HANDLE) {
					this->_f.pVkDestroyImageView(this->_hDevice, pTextureData->hImageView, nullptr);
					pTextureData->hImageView = VK_NULL_HANDLE;
				}

				if (pTextureData->hMemory != VK_NULL_HANDLE) {
					this->_f.pVkFreeMemory(this->_hDevice, pTextureData->hMemory, nullptr);
					pTextureData->hMemory = VK_NULL_HANDLE;
				}

				if (pTextureData->hImage != VK_NULL_HANDLE) {
					this->_f.pVkDestroyImage(this->_hDevice, pTextureData->hImage, nullptr);
					pTextureData->hImage = VK_NULL_HANDLE;
				}

				return;
			}


			VkDeviceMemory Backend::allocateMemory(const VkMemoryRequirements* pRequirements, VkMemoryPropertyFlagBits properties) const {
				VkMemoryAllocateInfo memAllocInfo{};
				memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memAllocInfo.allocationSize = pRequirements->size;
				memAllocInfo.memoryTypeIndex = UINT32_MAX;

				for (uint32_t i = 0u; i < this->_memoryProperties.memoryTypeCount; i++) {
					const bool hasProperties = (this->_memoryProperties.memoryTypes[i].propertyFlags & properties) == static_cast<VkMemoryPropertyFlags>(properties);

					if (hasProperties && pRequirements->memoryTypeBits & (1 << i)) {
						memAllocInfo.memoryTypeIndex = i;
					}

				}

				if (memAllocInfo.memoryTypeIndex == UINT32_MAX) return VK_NULL_HANDLE;

				VkDeviceMemory hMemory = VK_NULL_HANDLE;
				
				if (this->_f.pVkAllocateMemory(this->_hDevice, &memAllocInfo, nullptr, &hMemory) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hMemory;
			}


			VkImageView Backend::createImageView(VkImage hImage) const {
				VkImageSubresourceRange imageSubresourceRange{};
				imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageSubresourceRange.levelCount = 1u;
				imageSubresourceRange.layerCount = 1u;

				VkImageViewCreateInfo imageViewCreateInfo{};
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.image = hImage;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
				imageViewCreateInfo.subresourceRange = imageSubresourceRange;

				VkImageView hImageView = VK_NULL_HANDLE;

				if (this->_f.pVkCreateImageView(this->_hDevice, &imageViewCreateInfo, nullptr, &hImageView) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hImageView;
			}


			VkDescriptorSet Backend::createDescriptorSet(VkImageView hImageView) const {
				VkDescriptorSetAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = this->_hDescriptorPool;
				allocInfo.descriptorSetCount = 1u;
				allocInfo.pSetLayouts = &this->_hDescriptorSetLayout;

				VkDescriptorSet hDescriptorSet = VK_NULL_HANDLE;

				if (this->_f.pVkAllocateDescriptorSets(this->_hDevice, &allocInfo, &hDescriptorSet) != VK_SUCCESS) return VK_NULL_HANDLE;

				VkDescriptorImageInfo descImageInfo{};
				descImageInfo.sampler = this->_hTextureSampler;
				descImageInfo.imageView = hImageView;
				descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				VkWriteDescriptorSet writeDescSet{};
				writeDescSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescSet.dstSet = hDescriptorSet;
				writeDescSet.descriptorCount = 1u;
				writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writeDescSet.pImageInfo = &descImageInfo;
				this->_f.pVkUpdateDescriptorSets(this->_hDevice, 1u, &writeDescSet, 0u, nullptr);

				return hDescriptorSet;
			}


			VkBuffer Backend::createBuffer(VkDeviceSize size) const {
				VkBufferCreateInfo bufferCreateInfo{};
				bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferCreateInfo.size = size;
				bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

				VkBuffer hBuffer = VK_NULL_HANDLE;

				if (this->_f.pVkCreateBuffer(this->_hDevice, &bufferCreateInfo, nullptr, &hBuffer) != VK_SUCCESS) return VK_NULL_HANDLE;

				return hBuffer;
			}


			bool Backend::uploadImage(VkImage hImage, VkBuffer hBuffer, uint32_t width, uint32_t height) const {
				
				if (!this->beginCommandBuffer(this->_hTextureCommandBuffer)) return false;
				
				VkImageMemoryBarrier copyBarrier{};
				copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				copyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				copyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copyBarrier.image = hImage;
				copyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyBarrier.subresourceRange.levelCount = 1;
				copyBarrier.subresourceRange.layerCount = 1;
				
				this->_f.pVkCmdPipelineBarrier(this->_hTextureCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &copyBarrier);

				VkBufferImageCopy region{};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.layerCount = 1u;
				region.imageExtent.width = width;
				region.imageExtent.height = height;
				region.imageExtent.depth = 1u;

				this->_f.pVkCmdCopyBufferToImage(this->_hTextureCommandBuffer, hBuffer, hImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &region);

				VkImageMemoryBarrier useBarrier{};
				useBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				useBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				useBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				useBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				useBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				useBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				useBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				useBarrier.image = hImage;
				useBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				useBarrier.subresourceRange.levelCount = 1u;
				useBarrier.subresourceRange.layerCount = 1u;
				
				this->_f.pVkCmdPipelineBarrier(this->_hTextureCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0u, 0u, nullptr, 0u, nullptr, 1u, &useBarrier);

				if (this->_f.pVkEndCommandBuffer(this->_hTextureCommandBuffer) != VK_SUCCESS) return false;

				VkSubmitInfo submitInfo{};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1u;
				submitInfo.pCommandBuffers = &this->_hTextureCommandBuffer;

				if (this->_f.pVkQueueSubmit(this->_hFirstGraphicsQueue, 1u, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) return false;

				if (this->_f.pVkQueueWaitIdle(this->_hFirstGraphicsQueue) != VK_SUCCESS) return false;

				return true;
			}


			bool Backend::resizeFrameDataVector(uint32_t size, VkViewport viewport) {
				// destroy old frame data to be safe
				this->_frameDataVector.resize(0u);
				this->_frameDataVector.resize(size);
				
				VkImage* const pImages = new VkImage[size]{};
				uint32_t tmpSize = size;

				if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, this->_phPresentInfo->pSwapchains[0], &tmpSize, pImages) != VK_SUCCESS || tmpSize != size) {
					delete[] pImages;
					this->_frameDataVector.resize(0u);

					return false;
				}

				for (uint32_t i = 0u; i < size; i++) {
					
					if (!this->_frameDataVector[i].create(
						this->_f, this->_hDevice, this->_hCommandPool, pImages[i], this->_hRenderPass, static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height),
						this->_memoryProperties, this->_hPipelineLayout, this->_hPipelineTexture, this->_hPipelinePassthrough
					)) {
						delete[] pImages;
						this->_frameDataVector.resize(0u);

						return false;
					}

				}

				delete[] pImages;

				return true;
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
			
				if (this->_f.pVkResetCommandBuffer(hCommandBuffer, 0u) != VK_SUCCESS) return false;

				VkCommandBufferBeginInfo cmdBufferBeginInfo{};
				cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				return this->_f.pVkBeginCommandBuffer(hCommandBuffer, &cmdBufferBeginInfo) == VK_SUCCESS;
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