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

			this->destroyBufferData(&this->_triangleListBufferData);
			this->destroyBufferData(&this->_pointListBufferData);

			if (this->_pVertexDeclaration) {
				this->_pVertexDeclaration->Release();
			}

		}


		constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 100u;
		constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg1);

				if (!this->_pDevice) return;

				constexpr D3DVERTEXELEMENT9 vertexElements[]{
					{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0u },
					{ 0u, 8u, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0u },
					D3DDECL_END()
				};

				if (!this->_pVertexDeclaration) {

					if (FAILED(this->_pDevice->CreateVertexDeclaration(vertexElements, &this->_pVertexDeclaration))) return;

				}

				this->destroyBufferData(&this->_pointListBufferData);

				if (!this->createBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return;

				this->destroyBufferData(&this->_triangleListBufferData);

				if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;

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

			if (!this->_pointListBufferData.pLocalVertexBuffer) {

				if (!this->mapBufferData(&this->_pointListBufferData)) return;

			}
			
			if (!this->_triangleListBufferData.pLocalVertexBuffer) {

				if (!this->mapBufferData(&this->_triangleListBufferData)) return;

			}

			return;
		}


		void Draw::endDraw(const Engine* pEngine) { 
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			if (this->_triangleListBufferData.pVertexBuffer->Unlock() == D3D_OK) {
				this->_triangleListBufferData.pLocalVertexBuffer = nullptr;
				this->drawBufferData(&this->_triangleListBufferData, D3DPT_TRIANGLELIST);
			}

			if (this->_pointListBufferData.pVertexBuffer->Unlock() == D3D_OK) {
				this->_pointListBufferData.pLocalVertexBuffer = nullptr;
				this->drawBufferData(&this->_pointListBufferData, D3DPT_POINTLIST);
			}

			this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
			
			return; 
		}


		void Draw::drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) {

			if (!this->_isInit || count % 3u) return;
			
			this->copyToBufferData(&this->_triangleListBufferData, corners, count, color);

			return;
		}


		void Draw::drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset) {
			
			if (!this->_isInit) return;

			this->copyToBufferData(&this->_pointListBufferData, coordinates, count, color, offset);

			return;
		}


		bool Draw::createBufferData(BufferData* pBufferData, uint32_t vertexCount) const {
			RtlSecureZeroMemory(pBufferData, sizeof(BufferData));

			constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZ | D3DFVF_DIFFUSE;
			
			if (FAILED(this->_pDevice->CreateVertexBuffer(vertexCount * sizeof(Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &pBufferData->pVertexBuffer, nullptr))) return false;

			pBufferData->vertexBufferSize = vertexCount * sizeof(Vertex);

			return true;
		}



		void Draw::destroyBufferData(BufferData* pBufferData) const {
			pBufferData->vertexBufferSize = 0u;
			pBufferData->curOffset = 0u;

			if (!pBufferData->pVertexBuffer) return;
				
			if (pBufferData->pLocalVertexBuffer) {
				pBufferData->pVertexBuffer->Unlock();
				pBufferData->pLocalVertexBuffer = nullptr;
			}

			pBufferData->pVertexBuffer->Release();
			pBufferData->pVertexBuffer = nullptr;

			return;
		}


		bool Draw::mapBufferData(BufferData* pBufferData) const {

			if (!pBufferData->pVertexBuffer) return false;

			return SUCCEEDED(pBufferData->pVertexBuffer->Lock(0u, pBufferData->vertexBufferSize, reinterpret_cast<void**>(&pBufferData->pLocalVertexBuffer), D3DLOCK_DISCARD));
		}


		void Draw::copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset) const {
			
			if (!pBufferData->pLocalVertexBuffer) return;
			
			const uint32_t newVertexCount = pBufferData->curOffset + count;

			if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize) {

				if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

			}

			for (uint32_t i = 0u; i < count; i++) {
				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, COLOR_ABGR_TO_ARGB(color)};
				memcpy(&(pBufferData->pLocalVertexBuffer[pBufferData->curOffset + i]), &curVertex, sizeof(Vertex));
			}

			pBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const {
			
			if (newVertexCount <= pBufferData->curOffset) return true;

			BufferData oldBufferData = *pBufferData;

			if (!this->createBufferData(pBufferData, newVertexCount)) {
				this->destroyBufferData(pBufferData);

				return false;
			}

			if (!this->mapBufferData(pBufferData)) return false;


			if (oldBufferData.pLocalVertexBuffer) {
				memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, oldBufferData.vertexBufferSize);
			}

			pBufferData->curOffset = oldBufferData.curOffset;

			this->destroyBufferData(&oldBufferData);

			return true;
		}


		void Draw::drawBufferData(BufferData* pBufferData, D3DPRIMITIVETYPE type) const {
			UINT primitiveCount{};

			switch (type) {
			case D3DPRIMITIVETYPE::D3DPT_POINTLIST:
				primitiveCount = pBufferData->curOffset;
				break;
			case D3DPRIMITIVETYPE::D3DPT_LINELIST:
				primitiveCount = pBufferData->curOffset / 2;
				break;
			case D3DPRIMITIVETYPE::D3DPT_LINESTRIP:
				primitiveCount = pBufferData->curOffset - 1;
				break;
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST:
				primitiveCount = pBufferData->curOffset / 3;
				break;
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLESTRIP:
			case D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN:
				primitiveCount = pBufferData->curOffset - 2;
				break;
			default:
				return;
			}
			
			if (this->_pDevice->SetStreamSource(0u, pBufferData->pVertexBuffer, 0u, sizeof(Vertex)) == D3D_OK) {
				this->_pDevice->DrawPrimitive(type, 0u, primitiveCount);
				pBufferData->curOffset = 0u;
			}

		}

	}

}

