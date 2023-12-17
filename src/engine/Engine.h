#pragma once
#include "IDraw.h"
#include "rgb.h"

// Class for drawing within a hook independent of graphics API.
// 
// 
// Example usage:
// 
// DirectX 9 EndScene hook:
// 
// 
// hax::dx9::tEndScene pGatewayToEndScene = 0xDEADBEEF;
//
// hax::dx9::Font font(&hax::dx9::charsets::medium);
// hax::dx9::Draw draw;
// hax::Engine engine(&draw);
// 
// void APIENTRY hkEndScene(LPDIRECT3DDEVICE9 pOriginalDevice) {
//     engine.beginDraw(pOriginalDevice);
// 
//     Vector2 pos1{ 100, 100 };
//     Vector2 pos2{ 300, 100 };
// 
//     engine.drawLine(&pos1, &pos2, 2, hax::rgb::red);
//     engine.drawString(&font, &pos1, "Hi!", hax::rgb::green);
// 
//     pGatewayToEndScene(pOriginalDevice);
// 
//     return;
// }
// 
// 
// DirectX 11 Present hook:
// 
// 
// hax::dx11::tPresent pGatewayToPresent = 0xDEADBEEF;
//
// hax::dx::Font font<hax::dx11::Vertex>(&hax::dx::charsets::medium);
// hax::dx11::Draw draw;
// hax::Engine engine(&draw);
// 
// HRESULT __stdcall hkPresent(IDXGISwapChain* pOriginalSwapChain, UINT syncInterval, UINT flags) {
//     engine.beginDraw(pOriginalSwapChain);
// 
//     Vector2 pos1{ 100, 100 };
//     Vector2 pos2{ 300, 100 };
// 
//     engine.drawLine(&pos1, &pos2, 2, hax::rgb::red);
//     engine.drawString(&font, &pos1, "Hi!", hax::rgb::green);
// 
//     pGatewayToPresent(pOriginalSwapChain, syncInterval, flags);
// 
//     return;
// }
// 
// 
// OpenGL 2 wglSwapBuffers hook:
// 
// 
// hax::ogl2::twglSwapBuffers pGatewayToWglSwapBuffers = 0xDEADBEEF;
//
// hax::ogl2::Font* pFont;
// hax::ogl2::Draw draw;
// hax::Engine engine(&draw);
// 
// BOOL APIENTRY hkwglSwapBuffers(HDC hDc) {
//  
//     if (!pFont) {
//         pFont = new hax::ogl2::Font(hDc, "Consolas", 10);
//     }
// 
//     engine.beginDraw(hDc);
// 
//     Vector2 pos1{ 100, 100 };
//     Vector2 pos2{ 300, 100 };
// 
//     engine.drawLine(&pos1, &pos2, 2, hax::rgb::red);
//     engine.drawString(pFont, &pos1, "Hi!", hax::rgb::green);
// 
//     engine.endDraw();
// 
//     pGatewayToWglSwapBuffers(hDc);
// 
//     return;
// }

namespace hax {

	class Engine {
	private:
		IDraw* const _pDraw;

	public:
		void* pHookArg1;
		const void* pHookArg2;
		float fWindowWidth;
		float fWindowHeight;

		// Sets the IDraw interface for drawing with a specific graphics API.
		//
		// Parameters:
		// 
		// [in] pDraw:
		// Pointer to an appropriate IDraw interface to draw within a hook.
		Engine(IDraw* pDraw);

		// Initializes drawing within a hook. Has to be called before any drawing calls.
		//
		// Parameters:
		// 
		// [in] pArg1:
		// The argument of the the hooked function.
		// For OpenGL 2 wglSwapBuffers hooks pass the HDC.
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
		void beginDraw(void* pArg1, const void* pArg2 = nullptr);
		
		// Ends drawing within a hook. Has to be called after any drawing calls.
		void endDraw();

		// Sets the current window size.
		//
		// Parameters:
		// 
		// 
		// [in] width:
		// Width of the window.
		// 
		// [in] height:
		// Height of the window.
		void setWindowSize(float width, float height);
		
		void setWindowSize(unsigned long width, unsigned long height);

		void setWindowSize(int width, int height);

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
		// [in] origin:
		// Coordinates of the bottom left corner of the first character of the text.
		// 
		// [in] text:
		// Text to be drawn. See length limitations of IDraw implementations.
		//
		// [in] color:
		// Color of the text.
		void drawString(const void* pFont, const Vector2* origin, const char* text, rgb::Color color) const;

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