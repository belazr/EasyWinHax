#pragma once
#include "AbstractDrawBuffer.h"
#include "rgb.h"
#include "..\vecmath.h"
#include <stdint.h>

// Interface the engine class uses for drawing with graphics APIs within a hook.
// The appropriate implementation should be instantiated and passed to an engine object.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		class IBackend {
		public:
			// Sets the arguments of the current call of the hooked function.
			//
			// Parameters:
			// 
			// [in] pArg1:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass the LPDIRECT3DDEVICE9.
			// For DirectX 11 Present hooks pass the IDXGISwapChain.
			// For Vulkan QueuePresentKHR hooks pass the VkQueue.
			//
			// [in] pArg2:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass nothing.
			// For DirectX 11 Present hooks pass nothing.
			// For Vulkan QueuePresentKHR hooks pass the VkPresentInfoKHR.
			//
			// [in] pArg3:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass nothing.
			// For DirectX 11 Present hooks pass nothing.
			// For Vulkan QueuePresentKHR hooks pass the device handle that was retrieved by vk::getVulkanInitData().
			virtual void setHookArguments(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr) = 0;

			// Initializes the backend. Should be called by an Engine object until success.
			// 
			// Return:
			// True on success, false on failure.
			virtual bool initialize() = 0;

			// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
			// 
			// Return:
			// True on success, false on failure.
			virtual bool beginFrame() = 0;

			// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
			virtual void endFrame() = 0;

			// Gets the resolution of the current frame. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [out] frameWidth:
			// Pointer that receives the current frame width in pixel.
			//
			// [out] frameHeight:
			// Pointer that receives the current frame height in pixel.
			virtual void getFrameResolution(float* frameWidth, float* frameHeight) = 0;

			// Gets a reference to the triangle list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
			// 
			// Return:
			// Pointer to the triangle list buffer.
			virtual AbstractDrawBuffer* getTriangleListBuffer() const = 0;

			// Gets a reference to the point list buffer of the backend. It is the responsibility of the backend to dispose of the buffer properly.
			// 
			// Return:
			// Pointer to the point list buffer.
			virtual AbstractDrawBuffer* getPointListBuffer() const = 0;
		};

	}

}