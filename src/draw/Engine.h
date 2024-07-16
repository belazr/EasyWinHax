#pragma once
#include "IBackend.h"
#include "font\Font.h"

// Class for drawing within a graphics API hook.

namespace hax {

	namespace draw {

		class Engine {
		private:
			IBackend* const _pBackend;

			bool _init;
			bool _frame;

			AbstractDrawBuffer* _pTriangleListBuffer;
			AbstractDrawBuffer* _pPointListBuffer;

		public:
			float frameWidth;
			float frameHeight;

			// Sets the IBackend interface for drawing with a specific graphics API.
			//
			// Parameters:
			// 
			// [in] pBackend:
			// Pointer to an appropriate IBackend interface to backend within a hook.
			Engine(IBackend* pBackend);

			// Inititalizes the backend if neccessary and starts a frame within a hook.
			// Has to be called before any drawing calls.
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
			void beginFrame(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr);

			// Ends a frame within a hook. Has to be called after any drawing calls.
			void endFrame();

			// Draws a line. The line is a filled rectangle with the ends perpendicular to the middle axis.
			//
			// Parameters:
			//
			// [in] pos1:
			// Screen coordinates of the middle of the beginning of the line.
			// 
			// [in] pos2:
			// Screen coordinates of the middle of the end of the line.
			// 
			// [in] width:
			// Width in pixels.
			// 
			// [in] color:
			// Color of the line.
			void drawLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const;

			// Draws a line. The line is a parallelogram with horizontal ends.
			//
			// Parameters:
			//
			// [in] pos1:
			// Screen coordinates of the middle of the beginning of the line.
			// 
			// [in] pos2:
			// Screen coordinates of the middle of the end of the line.
			// 
			// [in] width:
			// Width in pixels.
			// 
			// [in] color:
			// Color of the line.
			void drawPLine(const Vector2* pos1, const Vector2* pos2, float width, rgb::Color color) const;

			// Draws a filled rectangle with the sides parallel to the screen edges.
			//
			// Parameters:
			//
			// [in] pos:
			// Coordinates of the top left corner of the rectangle.
			// 
			// [in] width:
			// Width of the rectangle.
			// 
			// [in] height:
			// Height of the rectangle.
			// 
			// [in] color:
			// Color of the rectangle.
			void drawFilledRectangle(const Vector2* pos, float width, float height, rgb::Color color) const;

			// Draws text to the screen.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to an appropriate Font object. ogl2::Font* for OpenGL 2 hooks and dx::Font* for DirectX hooks.
			//
			// [in] pos:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn.
			//
			// [in] color:
			// Color of the text.
			void drawString(const font::Font* pFont, const Vector2* pos, const char* text, rgb::Color color) const;

			// Draws a parallelogram grid with horizontal bottom and top sides.
			//
			// Parameters:
			//
			// [in] bot:
			// Screen coordinates of the middle of the bottom side.
			// 
			// [in] top:
			// Screen coordinates of the middle of the top side.
			// 
			// [in] ratio:
			// Ratio of bottom/top side length to height of the parallelogram
			// 
			// [in] width:
			// Line width in pixels.
			// 
			// [in] color:
			// Line color.
			void drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, rgb::Color color) const;

			// Draws a 2D box grid.
			//
			// Parameters:
			//
			// [in] bot:
			// Screen coordinates of the bottom left and right corner of the box.
			// 
			// [in] top:
			// Screen coordinates of the top left and right corner of the box.
			// 
			// [in] width:
			// Line width in pixels.
			// 
			// [in] color:
			// Line color
			void draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, rgb::Color color) const;

			// Draws a 3D box grid.
			//
			// Parameters:
			//
			// [in] bot:
			// Screen coordinates of the four bottom corners of the box. The bottom surface is drawn in clockwise orientation.
			// 
			// [in] top:
			// Screen coordinates of the four corners of the box. The top surface is drawn in clockwise orientation.
			// 
			// [in] width:
			// Line width in pixels.
			// 
			// [in] color:
			// Line color.
			void draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, rgb::Color color) const;
		};

	}

}