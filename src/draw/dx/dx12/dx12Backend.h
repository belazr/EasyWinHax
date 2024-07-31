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
			};

		}

	}

}

