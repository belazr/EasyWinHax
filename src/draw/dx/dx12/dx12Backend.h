#pragma once
#include "..\..\IBackend.h"
#include "..\..\Vertex.h"
#include <d3d12.h>
#include <dxgi1_4.h>

// Class for drawing within a DirectX 12 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx12 {

			typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags);

			// Gets a copy of the vTable of the DirectX 12 swap chain used by the caller process.
			// 
			// Parameter:
			// 
			// [out] pSwapChainVTable:
			// Contains the devices vTable on success. See the dxgi1_4.h header for the offset of the Present function (typically 8).
			// 
			// [in] size:
			// Size of the memory allocated at the address pointed to by pSwapChainVTable.
			// See the dxgi1_4.h header for the actual size of the vTable. Has to be at least offset of the function needed + one.
			// 
			// Return:
			// True on success, false on failure.
			bool getDXGISwapChain3VTable(void** pSwapChainVTable, size_t size);

			class Backend : public IBackend {
			private:
				typedef struct ImageData {
					ID3D12Resource* pRenderTargetResource;
					D3D12_CPU_DESCRIPTOR_HANDLE hRenderTargetDescriptor;
					ID3D12CommandAllocator* pCommandAllocator;
				}ImageData;

				IDXGISwapChain3* _pSwapChain;

				HWND _hMainWindow;
				ID3D12Device* _pDevice;
				ID3D12CommandQueue* _pCommandQueue;
				ID3D12Fence* _pFence;
				ID3D12DescriptorHeap* _pRtvDescriptorHeap;
				ID3D12RootSignature* _pRootSignature;
				ID3D12PipelineState* _pTriangleListPipelineState;
				ID3D12GraphicsCommandList* _pCommandList;
				D3D12_VIEWPORT _viewport;

				ImageData* _pImageDataArray;
				uint32_t _imageCount;
				ImageData* _pCurImageData;

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
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;

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
				bool createRootSignature();
				ID3D12PipelineState* createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) const;
				bool resizeImageDataArray(uint32_t imageCount, DXGI_FORMAT format);
				void destroyImageDataArray();
				void destroyImageData(ImageData* pImageData) const;
				bool getCurrentViewport(D3D12_VIEWPORT* pViewport) const;
			};

		}

	}

}

