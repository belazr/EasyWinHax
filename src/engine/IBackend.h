#pragma once
#include "rgb.h"
#include "..\vecmath.h"
#include <Windows.h>
#include <stdint.h>

// Interface the engine class uses for drawing with graphics APIs within a hook.
// The appropriate implementation should be instantiated and passed to an engine object.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	class IBackend {
	public:
		// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
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
		virtual void beginFrame(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr) = 0;

		// Ends the current frame within a hook. Should be called by an Engine object.
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

		// Draws a filled triangle list. Should be called by an Engine object.
		// 
		// Parameters:
		// 
		// [in] corners:
		// Screen coordinates of the corners of the triangles in the list.
		// The three corners of the first triangle have to be in clockwise order. From there on the orientation of the triangles has to alternate.
		// 
		// [in] count:
		// Count of the corners of the triangles in the list. Has to be divisble by three.
		// 
		// [in] color:
		// Color of each triangle.
		virtual void drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) = 0;

		// Draws a point list. Should be called by an Engine object.
		// 
		// Parameters:
		// 
		// [in] coordinate:
		// Screen coordinates of the points in the list.
		// 
		// [in] count:
		// Count of the points in the list..
		// 
		// [in] color:
		// Color of each point.
		//
		// [in] offest:
		// Offset by which each point is drawn.
		virtual void drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) = 0;
	};

}
