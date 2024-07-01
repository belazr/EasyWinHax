#include "dx9Draw.h"
#include "..\..\font\Font.h"
#include "..\..\Engine.h"

namespace hax {

	namespace dx9 {

		bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
			IDirect3D9* const pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

			if (!pDirect3D9) return false;

			D3DPRESENT_PARAMETERS d3dpp{};
			d3dpp.hDeviceWindow = GetDesktopWindow();
			d3dpp.Windowed = TRUE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

			IDirect3DDevice9* pDirect3D9Device = nullptr;

			HRESULT hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

			if (hResult != D3D_OK) {
				pDirect3D9->Release();

				return false;
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


		Draw::Draw() : _pDevice{}, _pOriginalVertexDeclaration{}, _pVertexDeclaration{}, _pointListBufferData {}, _triangleListBufferData{}, _viewport{}, _isInit {} {}


		Draw::~Draw() {

			if (this->_pointListBufferData.pBuffer) {
				this->_pointListBufferData.pBuffer->Unlock();
				this->_pointListBufferData.pBuffer->Release();
			}

		}


		constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZ | D3DFVF_DIFFUSE;
		constexpr UINT INITIAL_POINT_LIST_BUFFER_SIZE = sizeof(Vertex) * 1000;
		constexpr UINT INITIAL_TRIANGLE_LIST_BUFFER_SIZE = sizeof(Vertex) * 100;
		
		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg1);

				if (!this->_pDevice) return;

				if (!createVertexBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return;

				if (!createVertexBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;

				constexpr D3DVERTEXELEMENT9 vertexElements[]{
					{ 0, 0,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0 },
					{ 0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
					D3DDECL_END()
				};

				this->_pDevice->CreateVertexDeclaration(vertexElements, &this->_pVertexDeclaration);

				this->_isInit = true;
			}

			D3DVIEWPORT9 curViewport{};
			this->_pDevice->GetViewport(&curViewport);

			if (this->_viewport.Width != curViewport.Width || this->_viewport.Height != curViewport.Height) {
				this->_viewport = curViewport;
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}
			
			this->_pDevice->GetVertexDeclaration(&this->_pOriginalVertexDeclaration);
			this->_pDevice->SetVertexDeclaration(this->_pVertexDeclaration);

			// locking the buffers is expensive so it is just done once per frame if no resize is necessary
			this->_pointListBufferData.pBuffer->Lock(0u, this->_pointListBufferData.size, reinterpret_cast<void**>(&this->_pointListBufferData.pLocalBuffer), D3DLOCK_DISCARD);
			this->_triangleListBufferData.pBuffer->Lock(0u, this->_triangleListBufferData.size, reinterpret_cast<void**>(&this->_triangleListBufferData.pLocalBuffer), D3DLOCK_DISCARD);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) { 
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			// draw all the buffers at once to save api calls
			if (this->_triangleListBufferData.pBuffer->Unlock() == D3D_OK) {
				this->_triangleListBufferData.pLocalBuffer = nullptr;
				this->drawVertexBuffer(&this->_triangleListBufferData, D3DPT_TRIANGLELIST);
			}

			if (this->_pointListBufferData.pBuffer->Unlock() == D3D_OK) {
				this->_pointListBufferData.pLocalBuffer = nullptr;
				this->drawVertexBuffer(&this->_pointListBufferData, D3DPT_POINTLIST);
			}

			this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
			
			return; 
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit || count % 3) return;
			
			this->copyToVertexBuffer(&this->_triangleListBufferData, corners, count, color);

			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {
			
			if (!this->_isInit || !pFont) return;

			const font::Font* const pCurFont = reinterpret_cast<const font::Font*>(pFont);

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;
				
				const font::CharIndex index = font::charToCharIndex(c);
				const font::Char* pCurChar = &pCurFont->chars[index];

				if (pCurChar) {
					// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
					const Vector2 curPos{ pos->x + (pCurFont->width + 2.f) * i, pos->y - pCurFont->height };
					this->copyToVertexBuffer(&this->_pointListBufferData, pCurChar->body.coordinates, pCurChar->body.count, color, curPos);
					this->copyToVertexBuffer(&this->_pointListBufferData, pCurChar->outline.coordinates, pCurChar->outline.count, rgb::black, curPos);
				}

			}

			return;
		}


		bool Draw::createVertexBufferData(VertexBufferData* pVertexBufferData, UINT size) const {
			const HRESULT hResult = this->_pDevice->CreateVertexBuffer(size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &pVertexBufferData->pBuffer, nullptr);

			if (!pVertexBufferData->pBuffer) {
				pVertexBufferData->pLocalBuffer = nullptr;
				pVertexBufferData->size = 0u;
				pVertexBufferData->curOffset = 0u;

				return false;
			}

			if (hResult != D3D_OK) return false;

			pVertexBufferData->pLocalBuffer = nullptr;
			pVertexBufferData->size = size;
			pVertexBufferData->curOffset = 0u;

			return true;
		}


		void Draw::copyToVertexBuffer(VertexBufferData* pVertexBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) const {
			
			if (!pVertexBufferData->pLocalBuffer) return;
			
			const UINT sizeNeeded = (pVertexBufferData->curOffset + count) * sizeof(Vertex);

			if (sizeNeeded > pVertexBufferData->size) {

				if (!this->resizeVertexBuffer(pVertexBufferData, sizeNeeded * 2)) return;

			}

			for (UINT i = 0u; i < count; i++) {
				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, COLOR_ABGR_TO_ARGB(color)};
				memcpy(&(pVertexBufferData->pLocalBuffer[pVertexBufferData->curOffset + i]), &curVertex, sizeof(Vertex));
			}

			pVertexBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeVertexBuffer(VertexBufferData* pVertexBufferData, UINT newSize) const {
			const UINT bytesUsed = pVertexBufferData->curOffset * sizeof(Vertex);
			
			if (newSize < bytesUsed) return false;

			const VertexBufferData oldBufferData = *pVertexBufferData;

			if (!this->createVertexBufferData(pVertexBufferData, newSize)) return false;
			
			if (pVertexBufferData->pBuffer->Lock(0u, pVertexBufferData->size, reinterpret_cast<void**>(&pVertexBufferData->pLocalBuffer), D3DLOCK_DISCARD) != D3D_OK) return false;

			if (oldBufferData.pBuffer && oldBufferData.pLocalBuffer) {
				memcpy(pVertexBufferData->pLocalBuffer, oldBufferData.pLocalBuffer, bytesUsed);

				oldBufferData.pBuffer->Unlock();
				oldBufferData.pBuffer->Release();
			}

			return true;
		}


		void Draw::drawVertexBuffer(VertexBufferData* pVertexBufferData, D3DPRIMITIVETYPE type) const {
			UINT primitiveCount = 0u;

			switch (type) {
			case D3DPRIMITIVETYPE::D3DPT_POINTLIST:
				primitiveCount = pVertexBufferData->curOffset;
				break;
			case D3DPRIMITIVETYPE::D3DPT_LINELIST:
				primitiveCount = pVertexBufferData->curOffset / 2;
				break;
			case D3DPRIMITIVETYPE::D3DPT_LINESTRIP:
				primitiveCount = pVertexBufferData->curOffset - 1;
				break;
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST:
				primitiveCount = pVertexBufferData->curOffset / 3;
				break;
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLESTRIP:
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN:
				primitiveCount = pVertexBufferData->curOffset - 2;
				break;
			default:
				return;
			}
			
			if (this->_pDevice->SetStreamSource(0u, pVertexBufferData->pBuffer, 0u, sizeof(Vertex)) == D3D_OK) {
				this->_pDevice->DrawPrimitive(type, 0u, primitiveCount);
				pVertexBufferData->curOffset = 0u;
			}

		}

	}

}

