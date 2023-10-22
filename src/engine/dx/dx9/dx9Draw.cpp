#include "dx9Draw.h"
#include "dx9Vertex.h"
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


		Draw::Draw() : _pDevice{}, _pointListBufferData{}, _triangleListBufferData{}, _viewport{}, _world{}, _view{}, _projection{}, _identity{}, _ortho{}, _lightingState{}, _fvf{}, _isInit {} {}


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
				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg);

				if (!this->_pDevice) return;

				if (!createVertexBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return;

				if (!createVertexBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;

				this->_isInit = true;
			}

			if (this->saveMatricies()) {
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}

			this->_pDevice->SetTransform(D3DTS_WORLD, &this->_identity);
			this->_pDevice->SetTransform(D3DTS_VIEW, &this->_identity);
			this->_pDevice->SetTransform(D3DTS_PROJECTION, &this->_ortho);

			this->_pDevice->GetRenderState(D3DRS_LIGHTING, &this->_lightingState);
			this->_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
			
			this->_pDevice->GetFVF(&this->_fvf);
			this->_pDevice->SetFVF(D3DFVF_CUSTOM);

			// locking the buffers is expensive so it is just done once per frame if no resize is necessary
			this->_pointListBufferData.pBuffer->Lock(0, this->_pointListBufferData.size, reinterpret_cast<void**>(&this->_pointListBufferData.pLocalBuffer), D3DLOCK_DISCARD);
			this->_triangleListBufferData.pBuffer->Lock(0, this->_triangleListBufferData.size, reinterpret_cast<void**>(&this->_triangleListBufferData.pLocalBuffer), D3DLOCK_DISCARD);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) { 
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			// draw all the buffers at once to save api calls
			if (this->_pointListBufferData.pBuffer->Unlock() == D3D_OK) {
				this->_pointListBufferData.pLocalBuffer = nullptr;
				this->drawVertexBuffer(&this->_pointListBufferData, D3DPT_POINTLIST);
			}

			if (this->_triangleListBufferData.pBuffer->Unlock() == D3D_OK) {
				this->_triangleListBufferData.pLocalBuffer = nullptr;
				this->drawVertexBuffer(&this->_triangleListBufferData, D3DPT_TRIANGLELIST);
			}

			this->_pDevice->SetFVF(this->_fvf);

			this->_pDevice->SetRenderState(D3DRS_LIGHTING, this->_lightingState);

			this->_pDevice->SetTransform(D3DTS_WORLD, &this->_world);
			this->_pDevice->SetTransform(D3DTS_VIEW, &this->_view);
			this->_pDevice->SetTransform(D3DTS_PROJECTION, &this->_projection);
			
			return; 
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit || count % 3) return;
			
			this->copyToVertexBuffer(&this->_triangleListBufferData, corners, count, color);

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


		bool Draw::saveMatricies() {
			D3DVIEWPORT9 curViewport{};
			const HRESULT res = this->_pDevice->GetViewport(&curViewport);

			if (res != D3D_OK || (this->_viewport.Width == curViewport.Width && this->_viewport.Height == curViewport.Height)) return false;

			this->_pDevice->GetTransform(D3DTS_WORLD, &this->_world);
			this->_pDevice->GetTransform(D3DTS_VIEW, &this->_view);
			this->_pDevice->GetTransform(D3DTS_PROJECTION, &this->_projection);

			this->_identity = { { { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f } } };

			const float viewLeft = static_cast<const float>(curViewport.X);
			const float viewRight = static_cast<const float>(curViewport.X + curViewport.Width);
			const float viewTop = static_cast<const float>(curViewport.Y);
			const float viewBottom = static_cast<const float>(curViewport.Y + curViewport.Height);

			this->_ortho = {
				{
					{
						2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f,
						0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f,
						0.f, 0.f, .5f, 0.f,
						(viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f
					}
				}
			};

			this->_viewport = curViewport;

			return true;
		}


		void Draw::copyToVertexBuffer(VertexBufferData* pVertexBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) const {
			const UINT sizeNeeded = (pVertexBufferData->curOffset + count) * sizeof(Vertex);

			if (sizeNeeded > pVertexBufferData->size) {

				if (!this->resizeVertexBuffer(pVertexBufferData, sizeNeeded * 2)) return;

			}

			for (UINT i = 0; i < count; i++) {
				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
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
			
			if (this->_pDevice->SetStreamSource(0, pVertexBufferData->pBuffer, 0, sizeof(Vertex)) == D3D_OK) {
				this->_pDevice->DrawPrimitive(type, 0, primitiveCount);
				pVertexBufferData->curOffset = 0;
			}

		}

	}

}

