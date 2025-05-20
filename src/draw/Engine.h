#pragma once
#include "Font.h"
#include "IBackend.h"
#include "SolidDrawBuffer.h"
#include "TextureDrawBuffer.h"

// Class for drawing within a graphics API hook.

namespace hax {

	namespace draw {

		typedef enum Alignment {
			TOP_LEFT, TOP_CENTER, TOP_RIGHT,
			CENTER_LEFT, CENTER, CENTER_RIGHT,
			BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT
		}Alignment;

		class Engine {
		private:
			TextureDrawBuffer _textureDrawBuffer;
			SolidDrawBuffer _solidDrawBuffer;

			Font _font;

			IBackend* const _pBackend;

			bool _init;
			bool _frame;

		public:
			float frameWidth;
			float frameHeight;

			// Sets the IBackend interface for drawing with a specific graphics API.
			//
			// Parameters:
			// 
			// [in] pBackend:
			// Pointer to an appropriate IBackend interface to backend within a hook.
			Engine(IBackend* pBackend, Font font);

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
			// ID of the internal texture structure in VRAM that can be passed to drawTexture. 0 on failure.
			TextureId loadTexture(const Color* data, uint32_t width, uint32_t height);

			// Inititalizes the backend if neccessary and starts a frame within a hook.
			// Has to be called before any drawing calls.
			//
			// Parameters:
			// 
			// [in] pArg1:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass the LPDIRECT3DDEVICE9.
			// For DirectX 10 Present hooks pass the IDXGISwapChain*.
			// For DirectX 11 Present hooks pass the IDXGISwapChain*.
			// For DirectX 12 Present hooks pass the IDXGISwapChain3*.
			// For Vulkan QueuePresentKHR hooks pass the VkPresentInfoKHR.
			//
			// [in] pArg2:
			// The argument of the the hooked function.
			// For OpenGL 2 wglSwapBuffers hooks pass nothing.
			// For DirectX 9 EndScene hooks pass nothing.
			// For DirectX 10 EndScene hooks pass nothing.
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
			void drawLine(const Vector2* pos1, const Vector2* pos2, float width, Color color);

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
			void drawPLine(const Vector2* pos1, const Vector2* pos2, float width, Color color);

			// Draws a filled rectangle with the sides parallel to the screen edges.
			//
			// Parameters:
			//
			// [in] pos:
			// Position of the rectangle.
			// 
			// [in] alignmet:
			// Aligment of the rectangle relative to the position.
			// 
			// [in] width:
			// Width of the rectangle.
			// 
			// [in] height:
			// Height of the rectangle.
			// 
			// [in] color:
			// Color of the rectangle. Color format: DirectX 9 -> argb, DirectX 11 -> abgr, OpenGL 2 -> abgr, Vulkan: application dependent
			void drawFilledRectangle(const Vector2* pos, Alignment alignment, float width, float height, Color color);

			// Draws a loaded texture.
			//
			// Parameters:
			//
			// [in] id:
			// The ID of the texture returned by loadTexture.
			//
			// [in] pos:
			// Position of the texture.
			// 
			// [in] alignmet:
			// Aligment of the texture relative to the position.
			// 
			// [in] width:
			// Width of the drawn texture in pixels.
			// 
			// [in] height:
			// Height of the drawn texture in pixels.
			void drawTexture(TextureId textureId, const Vector2* pos, Alignment alignment, float width, float height);

			// Draws text to the screen.
			//
			// Parameters:
			// 
			// [in] pos:
			// Position of the string.
			// 
			// [in] alignmet:
			// Aligment of the string relative to the position.
			// 
			// [in] text:
			// Text to be drawn.
			// 
			// [in] size
			// Size of the text. Avoid very small (< 12) and very large (> 48) sizes for better readability.
			//
			// [in] color:
			// Color of the text.
			void drawString(const Vector2* pos, Alignment alignment, const char* text, uint32_t size, Color color);

			// Returns the height of a string when drawn to the screen.
			//
			// Parameters:
			// [in] size:
			// Size of the string.
			// 
			// Return:
			// Height of a string.
			float getStringHeight(uint32_t size);

			// Returns the dimensions of a string when drawn to the screen.
			//
			// Parameters:
			// [in] text:
			// Text to be drawn.
			// 
			// [in] size:
			// Size of the string.
			// 
			// Return:
			// 2D Vector of the string dimensions. X-component is width, y-component is height.
			Vector2 getStringDimensions(const char* text, uint32_t size) const;

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
			void drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, Color color);

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
			void draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, Color color);

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
			void draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, Color color);

			private:
				Vector2 align(const Vector2* pos, Alignment alignment, float width, float height);
		};

	}

}