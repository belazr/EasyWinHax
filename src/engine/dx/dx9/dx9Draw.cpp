#include "dx9Draw.h"
#include "dx9Vertex.h"
#include "..\..\Engine.h"
#include "..\..\..\Bench.h"

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


		Draw::Draw() : _pDevice{}, _pointListBufferData{}, _isInit {} {}


		Draw::~Draw() {

			if (this->_pointListBufferData.pBuffer) {
				this->_pointListBufferData.pBuffer->Unlock();
				this->_pointListBufferData.pBuffer->Release();
			}

		}


		constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZRHW | D3DFVF_DIFFUSE;
		constexpr UINT INITIAL_POINT_LIST_BUFFER_SIZE = sizeof(Vertex) * 1000;
		
		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg);

				if (!this->_pDevice) return;

				if (!createVertexBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return;

				this->_isInit = true;
			}

			D3DVIEWPORT9 viewport{};
			HRESULT const res = this->_pDevice->GetViewport(&viewport);

			if (res == D3D_OK) {
				pEngine->setWindowSize(viewport.Width, viewport.Height);
			}

			this->_pDevice->SetFVF(D3DFVF_CUSTOM);

			// locking the buffer is expensive so it is just done once per frame if no resize is necessary
			this->_pointListBufferData.pBuffer->Lock(0, this->_pointListBufferData.size, reinterpret_cast<void**>(&this->_pointListBufferData.pLocalBuffer), D3DLOCK_DISCARD);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) { 
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			if (this->_pointListBufferData.pBuffer->Unlock() == D3D_OK) {
				this->_pointListBufferData.pLocalBuffer = nullptr;

				// draw the whole point list buffer at once
				this->drawVertexBuffer(&this->_pointListBufferData, D3DPT_POINTLIST);
			}
			
			return; 
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit || count % 3) return;
			
			Vertex* const data = reinterpret_cast<Vertex*>(new BYTE[count * sizeof(Vertex)]);

			if (!data) return;

			for (UINT i = 0; i < count; i++) {
				data[i] = { corners[i], color };
			}

			// triangle count is vertex count / 3
			this->_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3, data, sizeof(Vertex));
			delete[] data;

			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {
			
			if (!this->_isInit || !pFont) return;

			const dx::Font* const pDxFont = reinterpret_cast<const dx::Font*>(pFont);

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;
				
				const dx::CharIndex index = dx::charToCharIndex(c);
				const dx::Fontchar* pCurChar = &pDxFont->chars[index];

				if (pCurChar && pCurChar->pixel) {
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pDxFont->width + 1) * i, pos->y - pDxFont->height };
					this->copyToVertexBuffer(&this->_pointListBufferData, pCurChar->pixel, pCurChar->pixelCount, color, curPos);
				}

			}

			return;
		}


		bool Draw::copyToVertexBuffer(VertexBufferData* pVertexBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) const {
			const UINT sizeNeeded = (pVertexBufferData->curOffset + count) * sizeof(Vertex);

			if (sizeNeeded > pVertexBufferData->size) {

				if (!this->resizeVertexBuffer(pVertexBufferData, sizeNeeded * 2)) return false;

			}

			for (UINT i = 0; i < count; i++) {
				Vertex curVertex({ data[i].x + offset.x, data[i].y + offset.y }, color);
				memcpy(&(pVertexBufferData->pLocalBuffer[pVertexBufferData->curOffset + i]), &curVertex, sizeof(Vertex));
			}

			pVertexBufferData->curOffset += count;

			return true;
		}


		bool Draw::resizeVertexBuffer(VertexBufferData* pVertexBufferData, UINT newSize) const {
			const UINT bytesUsed = pVertexBufferData->curOffset * sizeof(Vertex);
			
			if (newSize < bytesUsed) return false;

			const VertexBufferData oldBufferData = *pVertexBufferData;

			if (!createVertexBufferData(pVertexBufferData, newSize)) return false;
			
			if (pVertexBufferData->pBuffer->Lock(0, pVertexBufferData->size, reinterpret_cast<void**>(&pVertexBufferData->pLocalBuffer), D3DLOCK_DISCARD) != D3D_OK) return false;

			if (oldBufferData.pBuffer && oldBufferData.pLocalBuffer) {
				memcpy(pVertexBufferData->pLocalBuffer, oldBufferData.pLocalBuffer, bytesUsed);

				oldBufferData.pBuffer->Unlock();
				oldBufferData.pBuffer->Release();
			}

			return true;
		}


		bool Draw::createVertexBufferData(VertexBufferData* pVertexBufferData, UINT size) const {
			const HRESULT hResult = this->_pDevice->CreateVertexBuffer(size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &pVertexBufferData->pBuffer, nullptr);

			if (!pVertexBufferData->pBuffer) {
				pVertexBufferData->pLocalBuffer = nullptr;
				pVertexBufferData->size = 0;
				pVertexBufferData->curOffset = 0;

				return false;
			}

			if (hResult != D3D_OK) return false;

			pVertexBufferData->pLocalBuffer = nullptr;
			pVertexBufferData->size = size;
			pVertexBufferData->curOffset = 0;

			return true;
		}


		void Draw::drawVertexBuffer(VertexBufferData* pVertexBufferData, D3DPRIMITIVETYPE type) const {
			UINT primitiveCount = 0;

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
			
			if (this->_pDevice->SetStreamSource(0, this->_pointListBufferData.pBuffer, 0, sizeof(Vertex)) == D3D_OK) {
				this->_pDevice->DrawPrimitive(D3DPT_POINTLIST, 0, primitiveCount);
				pVertexBufferData->curOffset = 0;
			}

		}

	}

}

