#pragma once
#include "vkFrameData.h"
#include "..\IBackend.h"
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

				VkRenderPass _hRenderPass;
				uint32_t _graphicsQueueFamilyIndex;
				VkPhysicalDeviceMemoryProperties _memoryProperties;
				VkCommandPool _hCommandPool;
				VkCommandBuffer _hTextureCommandBuffer;
				VkSampler _hTextureSampler;
				VkDescriptorPool _hDescriptorPool;
				VkDescriptorSetLayout _hDescriptorSetLayout;
				VkPipelineLayout _hPipelineLayout;
				VkPipeline _hPipelineTexture;
				VkPipeline _hPipelinePassthrough;
				VkQueue _hFirstGraphicsQueue;
				VkViewport _viewport;

				Vector<FrameData> _frameDataVector;
				FrameData* _pCurFrameData;

				Vector<TextureData> _textures;

			public:
				Backend();

				~Backend();

				// Sets the arguments of the current call of the hooked function.
				//
				// Parameters:
				// 
				// [in] pArg2:
				// Pass the VkPresentInfoKHR*.
				//
				// [in] pArg2:
				// Pass the VkDevice that was retrieved by vk::getInitData().
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

				// Gets a reference to the texture buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the texture buffer backend.
				virtual IBufferBackend* getTextureBufferBackend() override;

				// Gets a reference to the solid buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the solid buffer backend.
				virtual IBufferBackend* getSolidBufferBackend() override;

				// Gets the resolution of the current frame. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [out] frameWidth:
				// Pointer that receives the current frame width in pixel.
				//
				// [out] frameHeight:
				// Pointer that receives the current frame height in pixel.
				virtual void getFrameResolution(float* frameWidth, float* frameHeight) const override;

			private:
				bool getProcAddresses();
				bool createRenderPass();
				bool getPhysicalDeviceProperties();
				bool createCommandPool();
				VkCommandBuffer allocCommandBuffer() const;
				bool createTextureSampler();
				bool createDescriptorPool();
				bool createDescriptorSetLayout();
				bool createPipelineLayout();
				VkPipeline createPipeline(const unsigned char* pFragmentShader, size_t fragmentShaderSize) const;
				VkShaderModule createShaderModule(const unsigned char* pShader, size_t size) const;
				VkImage createImage(uint32_t width, uint32_t height) const;
				void destroyTextureData(TextureData* pTextureData) const;
				VkDeviceMemory allocateMemory(const VkMemoryRequirements* pRequirements, VkMemoryPropertyFlagBits properties) const;
				VkImageView createImageView(VkImage hImage) const;
				VkDescriptorSet createDescriptorSet(VkImageView hImageView) const;
				VkBuffer createBuffer(VkDeviceSize size) const;
				bool uploadImage(VkImage hImage, VkBuffer hBuffer, uint32_t width, uint32_t height) const;
				bool resizeFrameDataVector(uint32_t size, VkViewport viewport);
				bool getCurrentViewport(VkViewport* pViewport) const;
				bool beginCommandBuffer(VkCommandBuffer hCommandBuffer) const;
				void beginRenderPass(VkCommandBuffer hCommandBuffer, VkFramebuffer hFramebuffer) const;
			};

		}

	}

}