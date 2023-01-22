#include "dx9Setup.h"
#include "..\proc.h"

namespace dx9 {
	
	int iWindowWidth;
	int iWindowHeight;
	float fWindowWidth;
	float fWindowHeight;

	static HWND getProcessWindow();

	bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
		
		if (!pDeviceVTable) return false;

		IDirect3D9* const pD3D = Direct3DCreate9(D3D_SDK_VERSION);

		if (!pD3D) return false;

		IDirect3DDevice9* pDummyDevice = nullptr;
		
		D3DPRESENT_PARAMETERS d3dpp{};
		d3dpp.Windowed = FALSE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.hDeviceWindow = dx9::getProcessWindow();

		HRESULT hResult = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

		if (hResult != S_OK) {
			d3dpp.Windowed = TRUE;
			hResult = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

			if (hResult != S_OK) {
				pD3D->Release();
				
				return false;
			}

		}

		memcpy_s(pDeviceVTable, size, *reinterpret_cast<void**>(pDummyDevice), size);
		
		pDummyDevice->Release();
		pD3D->Release();

		return true;
	}


	void setWindowSize(int width, int height) {
		iWindowWidth = width;
		iWindowHeight = height;
		fWindowWidth = static_cast<float>(width);
		fWindowHeight = static_cast<float>(height);

		return;
	}


	static HWND hProcWindow;

	static BOOL CALLBACK enumWindowsCallback(HWND handle, LPARAM lp);

	static HWND getProcessWindow() {
		EnumWindows(enumWindowsCallback, 0);
		
		if (hProcWindow) {
			RECT windowRect{};
			GetWindowRect(hProcWindow, &windowRect);
			setWindowSize(windowRect.right - windowRect.left, windowRect.bottom - windowRect.top);
		}

		return hProcWindow;
	}


	static BOOL CALLBACK enumWindowsCallback(HWND hWindow, LPARAM) {
		DWORD procId;
		GetWindowThreadProcessId(hWindow, &procId);

		if (GetCurrentProcessId() != procId) return TRUE;

		hProcWindow = hWindow;

		return FALSE;
	}

}