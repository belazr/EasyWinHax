#include "vkDraw.h"
#include "..\Engine.h"
#include "..\Vertex.h"
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

			if (!pVkDestroyDevice) return false;

			const VkInstance hInstance = createInstance(hVulkan);

			if (hInstance == VK_NULL_HANDLE) return false;

			VkPhysicalDevice hPhysicalDevice = getPhysicalDevice(hVulkan, hInstance);

			if (hPhysicalDevice == VK_NULL_HANDLE) return false;

			const VkDevice hDummyDevice = createDummyDevice(hVulkan, hPhysicalDevice);

			if (hDummyDevice == VK_NULL_HANDLE) return false;

			PFN_vkGetDeviceProcAddr pVkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(proc::in::getProcAddress(hVulkan, "vkGetDeviceProcAddr"));

			if (!pVkGetDeviceProcAddr) return false;

			initData->pVkQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkQueuePresentKHR"));

			const PFN_vkAcquireNextImageKHR pVkAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(pVkGetDeviceProcAddr(hDummyDevice, "vkAcquireNextImageKHR"));

			pVkDestroyDevice(hDummyDevice, nullptr);

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

			VkPhysicalDevice* const pPhysicalDevices = new VkPhysicalDevice[gpuCount]{};

			if (pVkEnumeratePhysicalDevices(hInstance, &gpuCount, pPhysicalDevices) != VkResult::VK_SUCCESS) return VK_NULL_HANDLE;

			VkPhysicalDevice hPhysicalDevice = VK_NULL_HANDLE;

			#pragma warning(push)
			#pragma warning(disable:6385)

			for (uint32_t i = 0u; i < gpuCount; i++) {
				VkPhysicalDeviceProperties properties{};
				pVkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
					hPhysicalDevice = pPhysicalDevices[i];
					break;
				}
			}

			#pragma warning(pop)

			delete[] pPhysicalDevices;

			return hPhysicalDevice;
		}


		static uint32_t getGraphicsQueueFamilyIndex(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice);

		static VkDevice createDummyDevice(HMODULE hVulkan, VkPhysicalDevice hPhysicalDevice) {
			const PFN_vkCreateDevice pVkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(proc::in::getProcAddress(hVulkan, "vkCreateDevice"));

			if (!pVkCreateDevice) return VK_NULL_HANDLE;
			
			const uint32_t queueFamily = getGraphicsQueueFamilyIndex(hVulkan, hPhysicalDevice);

			if (queueFamily == 0xFFFFFFFF) return VK_NULL_HANDLE;

			constexpr float QUEUE_PRIORITY = 1.f;

			VkDeviceQueueCreateInfo deviceQueueCreateInfo{};
			deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCreateInfo.queueFamilyIndex = queueFamily;
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
			_f{}, _hPhysicalDevice{}, _queueFamilyIndex{}, _memoryTypeIndex{}, _hDevice {}, _hRenderPass{},
			_hShaderModuleVert{}, _hShaderModuleFrag{}, _hDescriptorSetLayout{},
			_hPipelineLayout{}, _hPipeline{}, _bufferAlignment{}, _hCurCommandBuffer{},
			_pImageData{}, _imageCount{}, _triangleListBufferData{},
			_isInit{} {}


		Draw::~Draw() {
			destroyImageData();

			if (this->_hDevice == VK_NULL_HANDLE) return;

			if (this->_f.pVkDestroyRenderPass && this->_hRenderPass != VK_NULL_HANDLE) {
				this->_f.pVkDestroyRenderPass(this->_hDevice, this->_hRenderPass, nullptr);
			}

			if (this->_f.pVkDestroyShaderModule && this->_hShaderModuleVert != VK_NULL_HANDLE) {
				this->_f.pVkDestroyShaderModule(this->_hDevice, this->_hShaderModuleVert, nullptr);
			}

			if (this->_f.pVkDestroyShaderModule && this->_hShaderModuleFrag != VK_NULL_HANDLE) {
				this->_f.pVkDestroyShaderModule(this->_hDevice, this->_hShaderModuleFrag, nullptr);
			}

			if (this->_f.pVkDestroyDescriptorSetLayout && this->_hDescriptorSetLayout != VK_NULL_HANDLE) {
				this->_f.pVkDestroyDescriptorSetLayout(this->_hDevice, this->_hDescriptorSetLayout, nullptr);
			}

			if (this->_f.pVkDestroyPipelineLayout && this->_hPipelineLayout != VK_NULL_HANDLE) {
				this->_f.pVkDestroyPipelineLayout(this->_hDevice, this->_hPipelineLayout, nullptr);
			}

			if (this->_f.pVkDestroyPipeline && this->_hPipeline != VK_NULL_HANDLE) {
				this->_f.pVkDestroyPipeline(this->_hDevice, this->_hPipeline, nullptr);
			}
			
			if (this->_f.pVkDestroyBuffer && this->_triangleListBufferData.hVertexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, this->_triangleListBufferData.hVertexBuffer, nullptr);
			}

			if (this->_f.pVkFreeMemory && this->_triangleListBufferData.hVertexMemory != VK_NULL_HANDLE) {
				this->_f.pVkFreeMemory(this->_hDevice, this->_triangleListBufferData.hVertexMemory, nullptr);
			}

			if (this->_f.pVkDestroyBuffer && this->_triangleListBufferData.hIndexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, this->_triangleListBufferData.hIndexBuffer, nullptr);
			}

			if (this->_f.pVkFreeMemory && this->_triangleListBufferData.hIndexMemory != VK_NULL_HANDLE) {
				this->_f.pVkFreeMemory(this->_hDevice, this->_triangleListBufferData.hIndexMemory, nullptr);
			}
		}


		#define TEST_WIDTH 1366
		#define TEST_HEIGHT 768

		void Draw::beginDraw(Engine* pEngine) {
			const VkPresentInfoKHR* const pPresentInfo = reinterpret_cast<const VkPresentInfoKHR*>(pEngine->pHookArg2);
			this->_hDevice = reinterpret_cast<VkDevice>(pEngine->pHookArg3);

			if (!pPresentInfo || !this->_hDevice) return;

			if (!this->_isInit) {
				const HMODULE hVulkan = proc::in::getModuleHandle("vulkan-1.dll");

				if (!hVulkan) return;

				const PFN_vkDestroyInstance pVkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(proc::in::getProcAddress(hVulkan, "vkDestroyInstance"));

				if (!pVkDestroyInstance) return;

				const VkInstance hInstance = createInstance(hVulkan);

				if (hInstance == VK_NULL_HANDLE) return;

				if (!this->getInstanceProcAddresses(hVulkan, hInstance)) {
					pVkDestroyInstance(hInstance, nullptr);

					return;
				}

				this->_hPhysicalDevice = getPhysicalDevice(hVulkan, hInstance);
				pVkDestroyInstance(hInstance, nullptr);

				if (this->_hPhysicalDevice == VK_NULL_HANDLE) return;

				this->_queueFamilyIndex = getGraphicsQueueFamilyIndex(hVulkan, this->_hPhysicalDevice);

				if (this->_queueFamilyIndex == 0xFFFFFFFF) return;

				if (this->_hRenderPass == VK_NULL_HANDLE) {

					if (!this->createRenderPass()) return;

				}
				
				if (this->_hPipeline == VK_NULL_HANDLE) {

					if (!this->createPipeline()) return;

				}

				if (!this->createBufferData(&this->_triangleListBufferData, 3u)) return;

				this->_isInit = true;
			}

			if (!this->createImageData(pPresentInfo->pSwapchains[0])) return;

			for (uint32_t i = 0u; i < pPresentInfo->swapchainCount; i++) {
				const ImageData curImageData = this->_pImageData[pPresentInfo->pImageIndices[i]];

				if (curImageData.hCommandBuffer == VK_NULL_HANDLE) return;

				this->_hCurCommandBuffer = curImageData.hCommandBuffer;
				
				if (this->_f.pVkResetCommandBuffer(this->_hCurCommandBuffer, 0u) != VkResult::VK_SUCCESS) return;

				VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
				cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				cmdBufferBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

				if (this->_f.pVkBeginCommandBuffer(this->_hCurCommandBuffer, &cmdBufferBeginInfo) != VkResult::VK_SUCCESS) return;

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.renderPass = this->_hRenderPass;
				renderPassBeginInfo.framebuffer = curImageData.hFrameBuffer;
				renderPassBeginInfo.renderArea.extent.width = TEST_WIDTH;
				renderPassBeginInfo.renderArea.extent.height = TEST_HEIGHT;

				this->_f.pVkCmdBeginRenderPass(this->_hCurCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				BufferData* const tbd = &this->_triangleListBufferData;

				if (this->_f.pVkMapMemory(this->_hDevice, tbd->hVertexMemory, 0ull, tbd->vertexBufferSize, 0ull, reinterpret_cast<void**>(&tbd->pLocalVertexBuffer)) != VkResult::VK_SUCCESS) return;
				
				if (this->_f.pVkMapMemory(this->_hDevice, tbd->hIndexMemory, 0ull, tbd->indexBufferSize, 0ull, reinterpret_cast<void**>(&tbd->pLocalIndexBuffer)) != VkResult::VK_SUCCESS) return;
				
			}

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			VkMappedMemoryRange ranges[2]{};
			ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			ranges[0].memory = this->_triangleListBufferData.hVertexMemory;
			ranges[0].size = VK_WHOLE_SIZE;
			ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			ranges[1].memory = this->_triangleListBufferData.hIndexMemory;
			ranges[1].size = VK_WHOLE_SIZE;
			
			if(this->_f.pVkFlushMappedMemoryRanges(this->_hDevice, _countof(ranges), ranges) != VkResult::VK_SUCCESS) return;
			
			this->_f.pVkUnmapMemory(this->_hDevice, this->_triangleListBufferData.hVertexMemory);
			this->_triangleListBufferData.pLocalVertexBuffer = nullptr;
			
			this->_f.pVkUnmapMemory(this->_hDevice, this->_triangleListBufferData.hIndexMemory);
			this->_triangleListBufferData.pLocalIndexBuffer = nullptr;
			
			this->_f.pVkCmdBindPipeline(this->_hCurCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->_hPipeline);

			constexpr VkDeviceSize offset = 0;
			this->_f.pVkCmdBindVertexBuffers(this->_hCurCommandBuffer, 0u, 1u, &this->_triangleListBufferData.hVertexBuffer, &offset);
			this->_f.pVkCmdBindIndexBuffer(this->_hCurCommandBuffer, this->_triangleListBufferData.hIndexBuffer, 0ull, VK_INDEX_TYPE_UINT32);

			VkViewport viewport{};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = TEST_WIDTH;
			viewport.height = TEST_HEIGHT;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			this->_f.pVkCmdSetViewport(this->_hCurCommandBuffer, 0u, 1u, &viewport);

			float scale[2]{ 2.0f / TEST_WIDTH, 2.0f / TEST_HEIGHT };
			this->_f.pVkCmdPushConstants(this->_hCurCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u, sizeof(scale), scale);

			float translate[2]{ -1.f, -1.f };			
			this->_f.pVkCmdPushConstants(this->_hCurCommandBuffer, this->_hPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

			VkRect2D scissor{ { 0, 0 }, { TEST_WIDTH, TEST_HEIGHT } };
			this->_f.pVkCmdSetScissor(this->_hCurCommandBuffer, 0u, 1u, &scissor);

			this->_f.pVkCmdDrawIndexed(this->_hCurCommandBuffer, this->_triangleListBufferData.curOffset, 1u, 0u, 0u, 0u);

			this->_triangleListBufferData.curOffset = 0;
			
			this->_f.pVkCmdEndRenderPass(this->_hCurCommandBuffer);
			this->_f.pVkEndCommandBuffer(this->_hCurCommandBuffer);

			VkQueue hQueue = VK_NULL_HANDLE;
			this->_f.pVkGetDeviceQueue(this->_hDevice, this->_queueFamilyIndex, 0u, &hQueue);

			if (hQueue == VK_NULL_HANDLE) return;

			constexpr VkPipelineStageFlags stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pWaitDstStageMask = &stageMask;
			submitInfo.commandBufferCount = 1u;
			submitInfo.pCommandBuffers = &this->_hCurCommandBuffer;

			this->_f.pVkQueueSubmit(hQueue, 1u, &submitInfo, VK_NULL_HANDLE);

			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {

			return;
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {
			Vector2 test[]{
				{ 100, 100 },
				{ 200, 100 },
				{ 200, 200 }
			};

			corners = test;
			count = _countof(test);
			
			if (!this->_isInit || count % 3 || !this->_triangleListBufferData.pLocalVertexBuffer) return;

			for (UINT i = 0; i < count; i++) {
				const uint32_t curIndex = this->_triangleListBufferData.curOffset + i;

				Vertex curVertex{ { corners[i].x, corners[i].y }, color };
				memcpy(&(this->_triangleListBufferData.pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

				this->_triangleListBufferData.pLocalIndexBuffer[curIndex] = curIndex;
			}

			this->_triangleListBufferData.curOffset += count;

			return;
		}


		#define ASSIGN_PROC_ADDRESS(instance, pGetProcAddress, f) this->_f.pVk##f = reinterpret_cast<PFN_vk##f>(pGetProcAddress(instance, "vk"#f))

		bool Draw::getInstanceProcAddresses(HMODULE hVulkan, VkInstance hInstance) {
			const PFN_vkGetInstanceProcAddr pVkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(proc::in::getProcAddress(hVulkan, "vkGetInstanceProcAddr"));
			
			if (!pVkGetInstanceProcAddr) return false;

			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, GetPhysicalDeviceMemoryProperties);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, GetSwapchainImagesKHR);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateCommandPool);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyCommandPool);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, AllocateCommandBuffers);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, FreeCommandBuffers);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateImageView);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyImageView);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateFramebuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyFramebuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateRenderPass);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyRenderPass);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateShaderModule);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyShaderModule);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreatePipelineLayout);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyPipelineLayout);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateDescriptorSetLayout);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyDescriptorSetLayout);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateGraphicsPipelines);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyPipeline);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdBindPipeline);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CreateBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, GetBufferMemoryRequirements);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, AllocateMemory);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, BindBufferMemory);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, DestroyBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, FreeMemory);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, ResetCommandBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, BeginCommandBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, EndCommandBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdBeginRenderPass);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdEndRenderPass);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, MapMemory);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, UnmapMemory);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, FlushMappedMemoryRanges);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdBindVertexBuffers);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdBindIndexBuffer);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdSetViewport);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdPushConstants);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdDrawIndexed);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, CmdSetScissor);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, GetDeviceQueue);
			ASSIGN_PROC_ADDRESS(hInstance, pVkGetInstanceProcAddr, QueueSubmit);
			
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

		// glsl_shader.vert, compiled with:
		// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
		/*
		#version 450 core
		layout(location = 0) in vec2 aPos;
		layout(location = 1) in vec2 aUV;
		layout(location = 2) in vec4 aColor;
		layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

		out gl_PerVertex { vec4 gl_Position; };
		layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

		void main()
		{
			Out.Color = aColor;
			Out.UV = aUV;
			gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
		}
		*/
		static constexpr uint32_t GLSL_SHADER_VERT[] =
		{
			0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
			0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
			0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
			0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
			0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
			0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
			0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
			0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
			0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
			0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
			0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
			0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
			0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
			0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
			0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
			0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
			0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
			0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
			0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
			0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
			0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
			0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
			0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
			0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
			0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
			0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
			0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
			0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
			0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
			0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
			0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
			0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
			0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
			0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
			0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
			0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
			0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
			0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
			0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
			0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
			0x0000002d,0x0000002c,0x000100fd,0x00010038
		};

		// glsl_shader.frag, compiled with:
		// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
		/*
		#version 450 core
		layout(location = 0) out vec4 fColor;
		layout(set=0, binding=0) uniform sampler2D sTexture;
		layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
		void main()
		{
			fColor = In.Color * texture(sTexture, In.UV.st);
		}
		*/
		static constexpr uint32_t GLSL_SHADER_FRAG[] =
		{
			0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
			0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
			0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
			0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
			0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
			0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
			0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
			0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
			0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
			0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
			0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
			0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
			0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
			0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
			0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
			0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
			0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
			0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
			0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
			0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
			0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
			0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
			0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
			0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
			0x00010038
		};

		bool Draw::createPipeline() {

			if (this->_hShaderModuleVert == VK_NULL_HANDLE) {
				
				if (!this->createShaderModule(&this->_hShaderModuleVert, GLSL_SHADER_VERT, sizeof(GLSL_SHADER_VERT))) return false;

			}

			if (this->_hShaderModuleFrag == VK_NULL_HANDLE) {

				if (!this->createShaderModule(&this->_hShaderModuleFrag, GLSL_SHADER_FRAG, sizeof(GLSL_SHADER_FRAG))) return false;

			}
			
			if (this->_hPipelineLayout == VK_NULL_HANDLE) {

				if (!this->createPipelineLayout()) return false;

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
			attributeDesc[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDesc[1].offset = sizeof(Vector2);

			VkPipelineVertexInputStateCreateInfo vertexInfo{};
			vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInfo.vertexBindingDescriptionCount = 1u;
			vertexInfo.pVertexBindingDescriptions = &bindingDesc;
			vertexInfo.vertexAttributeDescriptionCount = 2u;
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

			return this->_f.pVkCreateGraphicsPipelines(this->_hDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &this->_hPipeline) == VkResult::VK_SUCCESS;
		}


		bool Draw::createShaderModule(VkShaderModule* pShaderModule, const uint32_t shader[], size_t size) {
			VkShaderModuleCreateInfo fragCreateInfo{};
			fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragCreateInfo.codeSize = size;
			fragCreateInfo.pCode = shader;

			return this->_f.pVkCreateShaderModule(this->_hDevice, &fragCreateInfo, nullptr, pShaderModule) == VkResult::VK_SUCCESS;
		}


		bool Draw::createPipelineLayout()
		{

			if (this->_hDescriptorSetLayout == VK_NULL_HANDLE) {
					
				if (!this->createDescriptorSetLayout()) return false;

			}

			VkPushConstantRange pushConstants{};
			pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstants.offset = sizeof(float) * 0;
			pushConstants.size = sizeof(float) * 4;

			VkPipelineLayoutCreateInfo layoutCreateInfo{};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 1;
			layoutCreateInfo.pSetLayouts = &this->_hDescriptorSetLayout;
			layoutCreateInfo.pushConstantRangeCount = 1;
			layoutCreateInfo.pPushConstantRanges = &pushConstants;
				
			return this->_f.pVkCreatePipelineLayout(this->_hDevice, &layoutCreateInfo, nullptr, &this->_hPipelineLayout) == VkResult::VK_SUCCESS;
		}


		bool Draw::createDescriptorSetLayout()
		{
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


		bool Draw::createBufferData(BufferData* pBufferData, size_t vertexCount) {
			
			if (pBufferData->hVertexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, pBufferData->hVertexBuffer, nullptr);
				pBufferData->vertexBufferSize = 0ull;
			}

			if (pBufferData->hVertexMemory != VK_NULL_HANDLE) {
				this->_f.pVkFreeMemory(this->_hDevice, pBufferData->hVertexMemory, nullptr);
			}

			VkDeviceSize vertexBufferSize = vertexCount * sizeof(Vertex);

			if (!this->createBuffer(&pBufferData->hVertexBuffer, &pBufferData->hVertexMemory, &vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

			pBufferData->vertexBufferSize = vertexBufferSize;

			if (pBufferData->hIndexBuffer != VK_NULL_HANDLE) {
				this->_f.pVkDestroyBuffer(this->_hDevice, pBufferData->hIndexBuffer, nullptr);
				pBufferData->indexBufferSize = 0ull;
			}

			if (pBufferData->hIndexMemory != VK_NULL_HANDLE) {
				this->_f.pVkFreeMemory(this->_hDevice, pBufferData->hIndexMemory, nullptr);
			}

			VkDeviceSize indexBufferSize = vertexCount * sizeof(uint32_t);

			if (!this->createBuffer(&pBufferData->hIndexBuffer, &pBufferData->hIndexMemory, &indexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) return false;

			pBufferData->indexBufferSize = indexBufferSize;
			
			return true;
		}

		bool Draw::createBuffer(VkBuffer* phBuffer, VkDeviceMemory* phMemory, VkDeviceSize* pSize, VkBufferUsageFlagBits usage) {

			if (!this->_bufferAlignment) {
				this->_bufferAlignment = 256ull;
			}
			
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
			VkPhysicalDeviceMemoryProperties memoryProperties{};

			this->_f.pVkGetPhysicalDeviceMemoryProperties(this->_hPhysicalDevice, &memoryProperties);

			for (uint32_t i = 0u; i < memoryProperties.memoryTypeCount; i++) {

				if ((memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && typeBits & (1 << i)) {
					return i;
				}
			}


			return 0xFFFFFFFF;
		}


		bool Draw::createImageData(VkSwapchainKHR hSwapchain) {
			uint32_t curImageCount = 0u;

			if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &curImageCount, nullptr) != VkResult::VK_SUCCESS) return false;

			if (!curImageCount) return false;

			if (curImageCount == this->_imageCount) return true;

			if (this->_pImageData) {
				this->destroyImageData();
			}

			this->_pImageData = new ImageData[curImageCount]{};
			this->_imageCount = curImageCount;
			
			VkImage* const pImages = new VkImage[this->_imageCount]{};

			if (this->_f.pVkGetSwapchainImagesKHR(this->_hDevice, hSwapchain, &this->_imageCount, pImages) != VkResult::VK_SUCCESS) {
				delete[] pImages;

				return false;
			}

			VkCommandPoolCreateInfo commandPoolCreateInfo{};
			commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			commandPoolCreateInfo.queueFamilyIndex = this->_queueFamilyIndex;

			VkCommandBufferAllocateInfo commandBufferAllocInfo{};
			commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferAllocInfo.commandBufferCount = 1;
			
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

			#pragma warning(push)
			#pragma warning(disable:6385)

			for (uint32_t i = 0; i < this->_imageCount; i++) {

				if (this->_f.pVkCreateCommandPool(this->_hDevice, &commandPoolCreateInfo, nullptr, &this->_pImageData[i].hCommandPool) != VkResult::VK_SUCCESS) continue;

				commandBufferAllocInfo.commandPool = this->_pImageData[i].hCommandPool;

				if (this->_f.pVkAllocateCommandBuffers(this->_hDevice, &commandBufferAllocInfo, &this->_pImageData[i].hCommandBuffer) != VkResult::VK_SUCCESS) continue;

				imageViewCreateInfo.image = pImages[i];

				if (this->_f.pVkCreateImageView(this->_hDevice, &imageViewCreateInfo, nullptr, &this->_pImageData[i].hImageView) != VkResult::VK_SUCCESS) continue;

				framebufferCreateInfo.pAttachments = &this->_pImageData[i].hImageView;
				this->_f.pVkCreateFramebuffer(this->_hDevice, &framebufferCreateInfo, nullptr, &this->_pImageData[i].hFrameBuffer);
			}

			#pragma warning(pop)

			delete[] pImages;

			return true;
		}


		void Draw::destroyImageData() {

			for (uint32_t i = 0u; i < this->_imageCount && this->_hDevice != VK_NULL_HANDLE; i++) {

				if (this->_pImageData[i].hFrameBuffer != VK_NULL_HANDLE) {
					this->_f.pVkDestroyFramebuffer(this->_hDevice, this->_pImageData[i].hFrameBuffer, nullptr);
				}
					
				if (this->_pImageData[i].hImageView != VK_NULL_HANDLE) {
					this->_f.pVkDestroyImageView(this->_hDevice, this->_pImageData[i].hImageView, nullptr);
				}

				if (this->_pImageData[i].hCommandBuffer != VK_NULL_HANDLE) {
					this->_f.pVkFreeCommandBuffers(this->_hDevice, this->_pImageData[i].hCommandPool, 1, &this->_pImageData[i].hCommandBuffer);
				}
					
				if (this->_pImageData[i].hCommandPool != VK_NULL_HANDLE) {
					this->_f.pVkDestroyCommandPool(this->_hDevice, this->_pImageData[i].hCommandPool, nullptr);
				}

			}

			delete[] this->_pImageData;
			this->_pImageData = nullptr;

			return;
		}

	}

}