#pragma once
#include "dx10DrawBuffer.h"
#include "..\..\IBackend.h"
#include "..\..\Vertex.h"
#include <d3d10_1.h>

// Class for drawing within a DirectX 10 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx10 {

			typedef HRESULT(__stdcall* tPresent)(IDXGISwapChain* pOriginalSwapChain, UINT syncInterval, UINT flags);

			// Gets a copy of the vTable of the DirectX 10 swap chain used by the caller process.
			// 
			// Parameter:
			// 
			// [out] pSwapChainVTable:
			// Contains the devices vTable on success. See the dxgi header for the offset of the Present function (typically 8).
			// 
			// [in] size:
			// Size of the memory allocated at the address pointed to by pDeviceVTable.
			// See the dxgi header for the actual size of the vTable. Has to be at least offset of the function needed + one.
			// 
			// Return:
			// True on success, false on failure.
			bool getD3D10SwapChainVTable(void** pSwapChainVTable, size_t size);

			class Backend : public IBackend {
			private:
				IDXGISwapChain* _pSwapChain;

			public:
				Backend();

				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass the IDXGISwapChain*.
				//
				// [in] pArg2:
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, void* pArg2 = nullptr) override;
			};

		}

	}

}