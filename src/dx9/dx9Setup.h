#pragma once
#include <d3d9.h>

// Setup for a DirectX 9 EndScene hook using a dummy device.
// Intended to be called from a thread started by a dll injected into a process.

namespace dx9 {
	
	typedef HRESULT(APIENTRY* tEndScene)(LPDIRECT3DDEVICE9 pDevice);

	// Globals for the window size. Integer and float types are provided to minimze conversion overhead within the hook.
	extern int iWindowWidth;
	extern int iWindowHeight;
	extern float fWindowWidth;
	extern float fWindowHeight;

	// Gets a copy of the vTable of the DirectX 9 device used by the caller process.
	// Parameter:
	// [out] pDeviceVTable: Conains the devices vTable on success. See the d3d9 header for the offset of the EndScene function (typically 42).
	// [in] size:			Size of the memory allocated at the address pointed to by pDeviceVTable.
	//						See the d3d9 header for the actual size of the vTable. Has to be at least offset of the function needed + one.
	// Return:				True on success, false on failure.
	bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size);

	// Sets the window size globals.
	// Parameters:
	// [in] width:	Windows width in pixels.
	// [in] height: Windows height in pixels.
	void setWindowSize(int width, int height);

}

