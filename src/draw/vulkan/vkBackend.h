#pragma once
#include "vkDrawBuffer.h"
#include "..\IBackend.h"
#include "..\Vertex.h"
#include "..\..\Vector.h"
#include <Windows.h>

namespace hax {

	namespace draw {

		namespace vk {

			typedef struct InitData {
				PFN_vkQueuePresentKHR pVkQueuePresentKHR;
				VkDevice hDevice;
			}InitData;

			bool getInitData(InitData* initData);

			class Backend : public IBackend {
			private:
				typedef struct ImageData {
					VkCommandBuffer hCommandBuffer;
					VkImageView hImageView;
					VkFramebuffer hFrameBuffer;
					DrawBuffer triangleListBuffer;
					DrawBuffer pointListBuffer;
					VkFence hFence;
				}ImageData;

				typedef struct TextureData {
					VkImage hImage;
					VkDeviceMemory hMemory;
					VkImageView hImageView;
					VkDescriptorSet hDescriptorSet;
				}TextureData;

				const VkPresentInfoKHR* _phPresentInfo;
				VkDevice _hDevice;

				HMODULE _hVulkan;
				HWND _hMainWindow;

				union {
					Functions _f;
					void* _fPtrs[sizeof(Functions) / sizeof(void*)];
				};

				uint32_t _graphicsQueueFamilyIndex;
				VkRenderPass _hRenderPass;
				VkCommandPool _hCommandPool;
				VkCommandBuffer _hTextureCommandBuffer;
				VkPipelineLayout _hPipelineLayout;
				VkPipeline _hTriangleListPipeline;
				VkPipeline _hPointListPipeline;
				VkPhysicalDeviceMemoryProperties _memoryProperties;
				VkQueue _hFirstGraphicsQueue;
				VkViewport _viewport;

				ImageData* _pImageDataArray;
				uint32_t _imageCount;
				ImageData* _pCurImageData;

				Vector<TextureData> _textures;

			public:
				Backend();

				~Backend();

				// Sets the arguments of the current call of the hooked function.
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
				// Pass the device handle that was retrieved by vk::getInitData().
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;

				// Loads a texture into VRAM.
				//
				// Parameters:
				// 
				// [in] data:
				// Texture colors in argb format.
				// 
				// [in] width:
				// Width of the texture.
				// 
				// [in] height:
				// Height of the texture.
				//
				// Return:
				// ID of the internal texture structure in VRAM that can be passed to DrawBuffer::append. 0 on failure.
				virtual TextureId loadTexture(const Color* data, uint32_t width, uint32_t height) override;

				// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

				// Gets a reference to the triangle list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
				// 
				// Return:
				// Pointer to the triangle list buffer.
				virtual AbstractDrawBuffer* getTriangleListBuffer() override;

				// Gets a reference to the point list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
				// 
				// Return:
				// Pointer to the point list buffer.
				virtual AbstractDrawBuffer* getPointListBuffer() override;

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

			private:
				bool getProcAddresses();
				bool getPhysicalDeviceProperties();
				bool createRenderPass();
				bool createCommandPool();
				VkCommandBuffer allocCommandBuffer() const;
				VkImageView createImageView(VkImage hImage) const;
				bool createPipelineLayout();
				VkDescriptorSetLayout createDescriptorSetLayout() const;
				VkPipeline createPipeline(VkPrimitiveTopology topology) const;
				VkShaderModule createShaderModule(const BYTE* shader, size_t size) const;
				void destroyTextureData(TextureData* pTextureData) const;
				bool createImageDataArray(uint32_t imageCount);
				void destroyImageDataArray();
				void destroyImageData(ImageData* pImageData) const;
				bool getCurrentViewport(VkViewport* pViewport) const;
				bool createFramebuffers(VkViewport viewport);
				void destroyFramebuffers();
				bool beginCommandBuffer(VkCommandBuffer hCommandBuffer) const;
				void beginRenderPass(VkCommandBuffer hCommandBuffer, VkFramebuffer hFramebuffer) const;
			};

		}

	}

}