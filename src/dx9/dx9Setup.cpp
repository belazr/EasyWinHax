#include "dx9Setup.h"
#include "..\proc.h"

namespace dx9 {
	
	int iWindowWidth;
	int iWindowHeight;
	float fWindowWidth;
	float fWindowHeight;

	static BOOL CALLBACK getWindowHandleCallback(HWND handle, LPARAM lp);

	bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
		HWND hWnd = nullptr;
		EnumWindows(getWindowHandleCallback, reinterpret_cast<LPARAM>(&hWnd));

		if (!hWnd) return false;

		RECT wndRect{};

		if (!GetWindowRect(hWnd, &wndRect)) return false;

		setWindowSize(wndRect.right - wndRect.left, wndRect.bottom - wndRect.top);

		IDirect3D9* const pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

		if (!pDirect3D9) return false;
		
		D3DPRESENT_PARAMETERS d3dpp{};
		d3dpp.hDeviceWindow = hWnd;
		d3dpp.Windowed = FALSE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		
		IDirect3DDevice9* pDirect3D9Device = nullptr;

		HRESULT hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

		if (hResult != S_OK) {
			d3dpp.Windowed = TRUE;
			hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

			if (hResult != S_OK) {
				pDirect3D9->Release();
				
				return false;
			}

		}

		if (memcpy_s(pDeviceVTable, size, *reinterpret_cast<void**>(pDirect3D9Device), size)) {
			pDirect3D9Device->Release();
			pDirect3D9->Release();
			
			return false;
		}
		
		pDirect3D9Device->Release();
		pDirect3D9->Release();

		return true;
	}


	void setWindowSize(int width, int height) {
		iWindowWidth = width;
		iWindowHeight = height;
		fWindowWidth = static_cast<float>(width);
		fWindowHeight = static_cast<float>(height);

		return;
	}


	static BOOL CALLBACK getWindowHandleCallback(HWND hWnd, LPARAM pArg) {
		DWORD procId = 0;
		GetWindowThreadProcessId(hWnd, &procId);

		if (GetCurrentProcessId() != procId || !procId) return TRUE;

		*reinterpret_cast<HWND*>(pArg) = hWnd;

		return FALSE;
	}

}