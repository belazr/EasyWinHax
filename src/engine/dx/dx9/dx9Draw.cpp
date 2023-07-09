#include "dx9Draw.h"
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

			if (hResult != S_OK) {
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


		Draw::Draw() : _pDevice{ nullptr }, _isInit{} {}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg);

				if (!this->_pDevice) return;

				this->_isInit = true;
			}

			D3DVIEWPORT9 viewport{};
			HRESULT const res = _pDevice->GetViewport(&viewport);

			if (res == S_OK) {
				pEngine->setWindowSize(viewport.Width, viewport.Height);
			}

			_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const { 
			UNREFERENCED_PARAMETER(pEngine);
			
			return; 
		}


		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {

			if (!this->_isInit) return;

			Vertex* const data = reinterpret_cast<Vertex*>(new BYTE[count * sizeof(Vertex)]);

			if (!data) return;

			for (UINT i = 0; i < count; i++) {
				data[i] = Vertex{ corners[i], color };
			}

			// triangle count is vertex count - 2
			this->_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, count - 2, data, sizeof(Vertex));
			delete[] data;

			return;
		}


		void Draw::drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const {
			
			if (!this->_isInit || !pFont) return;

			dx::Font<Vertex>* const pDx9Font = reinterpret_cast<dx::Font<Vertex>*>(pFont);

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;
				
				const dx::CharIndex index = dx::charToCharIndex(c);
				const dx::Fontchar * pCurChar = &pDx9Font->pCharset->chars[index];

				if (pCurChar && pCurChar->pixel) {
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pDx9Font->pCharset->width + 1) * i, pos->y - pDx9Font->pCharset->height };
					this->drawFontchar(pDx9Font, pCurChar, &curPos, index, color);
				}

			}

			return;
		}


		void Draw::drawFontchar(dx::Font<Vertex>* pDx9Font, const dx::Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const {

			if (!this->_isInit || !pDx9Font || index >= dx::CharIndex::MAX_CHAR || !pDx9Font->charVerticesArrays[index]) return;

			for (unsigned int i = 0; i < pChar->pixelCount; i++) {
				pDx9Font->charVerticesArrays[index][i] = Vertex{ pos->x + pChar->pixel[i].x, pos->y + pChar->pixel[i].y, color };
			}

			_pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, pChar->pixelCount, pDx9Font->charVerticesArrays[index], sizeof(Vertex));

			return;
		}

	}

}

