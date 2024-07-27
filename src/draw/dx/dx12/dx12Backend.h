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

			class Backend : public IBackend {
			};

		}

	}

}

