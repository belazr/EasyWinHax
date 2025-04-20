#pragma once
#include "dx12BufferBackend.h"
#include "..\..\IBackend.h"
#include "..\..\..\Vector.h"
#include <dxgi1_4.h>

// Class for drawing within a DirectX 12 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx12 {

			typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);

			typedef struct InitData {
				tPresent pPresent;
				ID3D12CommandQueue* pCommandQueue;
			}InitData;

			// Gets data to needed to setup drawing with an engine object in a DirectX 12 Present hook.
			// This function should be called from a thread of the DirectX 12 target application.
			// On success the InitData structure will contain a pointer to the Present function that should be hook
			// and a pointer to the command queue that should be passed to the engine objects beginFrame function.
			// See the Engine.h header and the DirectX12Hook example for more details.
			//
			// Parameters:
			// 
			// [out] pInitData:
			// Pointer to an empty InitData structure.
			// 
			// Return:
			// True on success, false on failure.
			bool getInitData(InitData* pInitData);

			class Backend : public IBackend {
			private:
				typedef struct ImageData {
					ID3D12CommandAllocator* pCommandAllocator;
					BufferBackend triangleListBuffer;
					BufferBackend pointListBuffer;
					BufferBackend textureTriangleListBuffer;
					HANDLE hEvent;
				}ImageData;

				IDXGISwapChain3* _pSwapChain;
				ID3D12CommandQueue* _pCommandQueue;

				HWND _hMainWindow;
				ID3D12Device* _pDevice;
				ID3D12DescriptorHeap* _pRtvDescriptorHeap;
				ID3D12DescriptorHeap* _pSrvDescriptorHeap;
				D3D12_CPU_DESCRIPTOR_HANDLE _hRtvHeapStartDescriptor;
				D3D12_CPU_DESCRIPTOR_HANDLE _hSrvHeapStartCpuDescriptor;
				D3D12_GPU_DESCRIPTOR_HANDLE _hSrvHeapStartGpuDescriptor;
				UINT _srvHeapDescriptorIncrementSize;
				ID3D12RootSignature* _pRootSignature;
				ID3D12PipelineState* _pTriangleListPipelineStatePassthrough;
				ID3D12PipelineState* _pPointListPipelineStatePassthrough;
				ID3D12PipelineState* _pTriangleListPipelineStateTexture;
				ID3D12Fence* _pFence;
				ID3D12GraphicsCommandList* _pCommandList;
				ID3D12Resource* _pRtvResource;
				D3D12_VIEWPORT _viewport;

				ImageData* _pImageDataArray;
				uint32_t _imageCount;
				UINT _curBackBufferIndex;
				ImageData* _pCurImageData;

				Vector<ID3D12Resource*> _textures;

			public:
				Backend();

				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass the IDXGISwapChain3*.
				//
				// [in] pArg2:
				// Pass the ID3D12CommandQueue* that was retrieved by dx12::getInitData().
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

				// Starts a frame within a hook. Should be called by an Engine object every frame at the beginning of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

				// Gets a reference to the triangle list buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the triangle list buffer backend.
				virtual IBufferBackend* getTriangleListBufferBackend() override;

				// Gets a reference to the point list buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the point list buffer backend.
				virtual IBufferBackend* getPointListBufferBackend() override;

				// Gets a reference to the texture triangle list buffer backend. It is the responsibility of the backend to dispose of the buffer backend properly.
				// 
				// Return:
				// Pointer to the texture triangle list buffer backend.
				virtual IBufferBackend* getTextureTriangleListBufferBackend() override;

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
				bool createDescriptorHeaps();
				bool createRootSignature();
				ID3D12PipelineState* createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology, D3D12_SHADER_BYTECODE pixelShader) const;
				bool createImageDataArray(uint32_t imageCount);
				void destroyImageDataArray();
				void destroyImageData(ImageData* pImageData) const;
				bool createRenderTargetView(DXGI_FORMAT format);
				bool getCurrentViewport(D3D12_VIEWPORT* pViewport) const;
			};

		}

	}

}

