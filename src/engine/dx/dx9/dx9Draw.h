#pragma once
#include "dx9Vertex.h"
#include "..\font\dxFonts.h"
#include "..\..\IDraw.h"
#include <d3d9.h>

// Class for drawing within a DirectX 9 EndScene hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace dx9 {

		typedef HRESULT(APIENTRY* tEndScene)(LPDIRECT3DDEVICE9 _pDevice);

		// Gets a copy of the vTable of the DirectX 9 device used by the caller process.
		// 
		// Parameter:
		// 
		// [out] pDeviceVTable:
		// Contains the devices vTable on success. See the d3d9 header for the offset of the EndScene function (typically 42).
		// 
		// [in] size:
		// Size of the memory allocated at the address pointed to by pDeviceVTable.
		// See the d3d9 header for the actual size of the vTable. Has to be at least offset of the function needed + one.
		// 
		// Return:
		// True on success, false on failure.
		bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size);

		typedef struct VertexBufferData {
			IDirect3DVertexBuffer9* pBuffer;
			Vertex* pLocalBuffer;
			UINT size;
			UINT curOffset;
		}VertexBufferData;

		class Draw : public IDraw {
		private:
			IDirect3DDevice9* _pDevice;
			VertexBufferData _pointListBufferData;
			VertexBufferData _triangleListBufferData;

			D3DVIEWPORT9 _viewport;
			D3DMATRIX _world;
			D3DMATRIX _view;
			D3DMATRIX _projection;
			D3DMATRIX _identity;
			D3DMATRIX _ortho;
			DWORD _lightingState;
			DWORD _fvf;

			bool _isInit;

		public:
			Draw();

			~Draw();

			// Initializes drawing within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void beginDraw(Engine* pEngine) override;

			// Ends drawing within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void endDraw(const Engine* pEngine) override;

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. For there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in]
			// Color of the triangle list.
			void drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) override;

			// Draws text to the screen. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to a dx::Font object.
			//
			// [in] origin:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn. See length limitations implementations.
			//
			// [in] color:
			// Color of the text.
			void drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) override;
			
		private:
			bool saveMatricies();
			void copyToVertexBuffer(VertexBufferData* pVertexBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const;
			bool resizeVertexBuffer(VertexBufferData* pVertexBufferData, UINT newSize) const;
			bool createVertexBufferData(VertexBufferData* pVertexBufferData, UINT size) const;
			void drawVertexBuffer(VertexBufferData* pVertexBufferData, D3DPRIMITIVETYPE type) const;

		};

	}
	
}


