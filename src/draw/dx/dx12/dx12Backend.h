#pragma once
#include "dx12DrawBuffer.h"
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

			typedef struct Dx12InitData {
				tPresent pPresent;
				ID3D12CommandQueue* pCommandQueue;
			}Dx12InitData;

			bool getDx12InitData(Dx12InitData* pInitData);

			class Backend : public IBackend {
			private:
				typedef struct ImageData {
					ID3D12CommandAllocator* pCommandAllocator;
					DrawBuffer triangleListBuffer;
					DrawBuffer pointListBuffer;
				}ImageData;

				IDXGISwapChain3* _pSwapChain;
				ID3D12CommandQueue* _pCommandQueue;

				HWND _hMainWindow;
				ID3D12Device* _pDevice;
				ID3D12DescriptorHeap* _pRtvDescriptorHeap;
				D3D12_CPU_DESCRIPTOR_HANDLE _hRtvHeapStartDescriptor;
				ID3D12RootSignature* _pRootSignature;
				ID3D12PipelineState* _pPipelineState;
				ID3D12GraphicsCommandList* _pCommandList;
				ID3D12Resource* _pRtvResource;
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
				bool createPipelineState(DXGI_FORMAT format);
				bool resizeImageDataArray(uint32_t imageCount);
				void destroyImageDataArray();
				void destroyImageData(ImageData* pImageData) const;
				bool getCurrentViewport(D3D12_VIEWPORT* pViewport) const;
			};

		}

	}

}

