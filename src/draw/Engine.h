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

			// Loads a texture into VRAM. Has to be called after beginFrame.
			//
			// Parameters:
			// 
			// [in] data:
			// Texture colors in argb format.
			// 
			// [in] width:
			// Width of the texture.
			// 
			// [in] height:
			// Height of the texture.
			//
			// Return:
			// Pointer to the internal texture structure in VRAM that can be passed to drawTexture. nullptr on failure.
			void* loadTexture(const Color* data, uint32_t width, uint32_t height);

			// Inititalizes the backend if neccessary and starts a frame within a hook.
			// Has to be called before any drawing calls.
			//
			// Parameters:
			// 
			// [in] pArg1:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass the LPDIRECT3DDEVICE9.
			// For DirectX 11 Present hooks pass the IDXGISwapChain*.
			// For DirectX 12 Present hooks pass the IDXGISwapChain3*.
			// For Vulkan QueuePresentKHR hooks pass the VkPresentInfoKHR.
			//
			// [in] pArg2:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass nothing.
			// For DirectX 11 Present hooks pass nothing.
			// For DirectX 12 Present hooks pass the ID3D12CommandQueue* that was retrieved by dx12::getInitData().
			// For Vulkan QueuePresentKHR hooks pass the VkDevice that was retrieved by vk::getInitData().
			void beginFrame(void* pArg1 = nullptr, void* pArg2 = nullptr);

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
			// Color of the line. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) const;

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
			// Color of the line. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawPLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) const;

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
			// Color of the rectangle. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawFilledRectangle(const Vector2* pos, float width, float height, Color color) const;

			// Draws a loaded texture.
			//
			// Parameters:
			//
			// [in] id:
			// The ID of the texture returned by loadTexture.
			//
			// [in] pos:
			// Coordinates of the top left corner of the texture.
			// 
			// [in] width:
			// Width of the drawn texture in pixels.
			// 
			// [in] height:
			// Height of the drawn texture in pixels.
			void drawTexture(void* id, const Vector2* pos, float width, float height) const;

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
			// Color of the text. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawString(const font::Font* pFont, const Vector2* pos, const char* text, Color color) const;

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
			// Line color. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, Color color) const;

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
			// Line color. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, Color color) const;

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
			// Line color. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, Color color) const;
		};

	}

}