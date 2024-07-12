#include "dx9Backend.h"

namespace hax {

	namespace draw {

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


			Backend::Backend() : _pDevice{}, _pOriginalVertexDeclaration{}, _pVertexDeclaration{}, _pointListBufferData{}, _triangleListBufferData{}, _viewport{} {}


			Backend::~Backend() {

				this->destroyBufferData(&this->_triangleListBufferData);
				this->destroyBufferData(&this->_pointListBufferData);

				if (this->_pVertexDeclaration) {
					this->_pVertexDeclaration->Release();
				}

			}


			void Backend::setHookArguments(void* pArg1, const void* pArg2, void* pArg3) {
				UNREFERENCED_PARAMETER(pArg2);
				UNREFERENCED_PARAMETER(pArg3);

				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pArg1);

				return;
			}

			
			bool Backend::initialize() {
				constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[]{
					{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0u },
					{ 0u, 8u, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0u },
					D3DDECL_END()
				};

				if (!this->_pVertexDeclaration) {

					if (FAILED(this->_pDevice->CreateVertexDeclaration(VERTEX_ELEMENTS, &this->_pVertexDeclaration))) return false;

				}

				this->destroyBufferData(&this->_pointListBufferData);

				constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 1000u;

				if (!this->createBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return false;

				this->destroyBufferData(&this->_triangleListBufferData);

				constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

				if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return false;

				return true;
			}


			bool Backend::beginFrame() {
				
				if(FAILED(this->_pDevice->GetViewport(&this->_viewport))) return false;

				if (!this->mapBufferData(&this->_pointListBufferData)) return false;

				if (!this->mapBufferData(&this->_triangleListBufferData)) return false;

				if (!this->_pOriginalVertexDeclaration) {

					if (FAILED(this->_pDevice->GetVertexDeclaration(&this->_pOriginalVertexDeclaration))) return false;

				}

				if (FAILED(this->_pDevice->SetVertexDeclaration(this->_pVertexDeclaration))) {
					this->_pOriginalVertexDeclaration->Release();
					this->_pOriginalVertexDeclaration = nullptr;

					return false;
				}

				return true;
			}


			void Backend::endFrame() {
				
				if (SUCCEEDED(this->_triangleListBufferData.pVertexBuffer->Unlock())) {
					this->_triangleListBufferData.pLocalVertexBuffer = nullptr;
					this->drawBufferData(&this->_triangleListBufferData, D3DPT_TRIANGLELIST);
				}

				if (SUCCEEDED(this->_pointListBufferData.pVertexBuffer->Unlock())) {
					this->_pointListBufferData.pLocalVertexBuffer = nullptr;
					this->drawBufferData(&this->_pointListBufferData, D3DPT_POINTLIST);
				}

				this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
				this->_pOriginalVertexDeclaration->Release();
				this->_pOriginalVertexDeclaration = nullptr;

				return;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = static_cast<float>(this->_viewport.Width);
				*frameHeight = static_cast<float>(this->_viewport.Height);

				return;
			}


			void Backend::drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) {

				if (count % 3u) return;

				this->copyToBufferData(&this->_triangleListBufferData, corners, count, color);

				return;
			}


			void Backend::drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset) {
				this->copyToBufferData(&this->_pointListBufferData, coordinates, count, color, offset);

				return;
			}


			bool Backend::createBufferData(BufferData* pBufferData, uint32_t vertexCount) const {
				RtlSecureZeroMemory(pBufferData, sizeof(BufferData));

				constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZ | D3DFVF_DIFFUSE;
				const UINT vertexBufferSize = vertexCount * sizeof(Vertex);

				if (FAILED(this->_pDevice->CreateVertexBuffer(vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &pBufferData->pVertexBuffer, nullptr))) return false;

				pBufferData->vertexBufferSize = vertexBufferSize;

				const UINT indexBufferSize = vertexCount * sizeof(uint32_t);

				if (FAILED(this->_pDevice->CreateIndexBuffer(indexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &pBufferData->pIndexBuffer, nullptr))) return false;

				pBufferData->indexBufferSize = indexBufferSize;

				return true;
			}


			void Backend::destroyBufferData(BufferData* pBufferData) const {

				if (pBufferData->pVertexBuffer) {
					pBufferData->pVertexBuffer->Unlock();
					pBufferData->pLocalVertexBuffer = nullptr;
					pBufferData->pVertexBuffer->Release();
					pBufferData->pVertexBuffer = nullptr;
				}

				if (pBufferData->pIndexBuffer) {
					pBufferData->pIndexBuffer->Unlock();
					pBufferData->pLocalIndexBuffer = nullptr;
					pBufferData->pIndexBuffer->Release();
					pBufferData->pIndexBuffer = nullptr;
				}

				pBufferData->vertexBufferSize = 0u;
				pBufferData->indexBufferSize = 0u;
				pBufferData->curOffset = 0u;

				return;
			}


			bool Backend::mapBufferData(BufferData* pBufferData) const {

				if (!pBufferData->pLocalVertexBuffer) {

					if (FAILED(pBufferData->pVertexBuffer->Lock(0u, pBufferData->vertexBufferSize, reinterpret_cast<void**>(&pBufferData->pLocalVertexBuffer), D3DLOCK_DISCARD))) return false;

				}

				if (!pBufferData->pLocalIndexBuffer) {

					if (FAILED(pBufferData->pIndexBuffer->Lock(0u, pBufferData->indexBufferSize, reinterpret_cast<void**>(&pBufferData->pLocalIndexBuffer), D3DLOCK_DISCARD))) return false;

				}

				return true;
			}


			void Backend::copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset) const {
				const uint32_t newVertexCount = pBufferData->curOffset + count;

				if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize || newVertexCount * sizeof(uint32_t) > pBufferData->indexBufferSize) {

					if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

				}

				if (!pBufferData->pLocalVertexBuffer || !pBufferData->pLocalIndexBuffer) return;

				const rgb::Color argbColor = COLOR_ABGR_TO_ARGB(color);

				for (uint32_t i = 0u; i < count; i++) {
					const uint32_t curIndex = pBufferData->curOffset + i;

					const Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, argbColor };
					memcpy(&(pBufferData->pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

					pBufferData->pLocalIndexBuffer[curIndex] = curIndex;
				}

				pBufferData->curOffset += count;

				return;
			}


			bool Backend::resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const {

				if (newVertexCount <= pBufferData->curOffset) return true;

				BufferData oldBufferData = *pBufferData;

				if (!this->createBufferData(pBufferData, newVertexCount)) {
					this->destroyBufferData(pBufferData);
					this->destroyBufferData(&oldBufferData);

					return false;
				}

				if (!this->mapBufferData(pBufferData)) {
					this->destroyBufferData(&oldBufferData);

					return false;
				}

				if (oldBufferData.pLocalVertexBuffer) {
					memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, oldBufferData.vertexBufferSize);
				}

				if (oldBufferData.pLocalIndexBuffer) {
					memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalIndexBuffer, oldBufferData.indexBufferSize);
				}

				pBufferData->curOffset = oldBufferData.curOffset;

				this->destroyBufferData(&oldBufferData);

				return true;
			}


			void Backend::drawBufferData(BufferData* pBufferData, D3DPRIMITIVETYPE type) const {
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

				if (FAILED(this->_pDevice->SetStreamSource(0u, pBufferData->pVertexBuffer, 0u, sizeof(Vertex)))) return;

				if (FAILED(this->_pDevice->SetIndices(pBufferData->pIndexBuffer))) return;

				if (FAILED(pBufferData->pVertexBuffer->Unlock())) return;

				pBufferData->pLocalVertexBuffer = nullptr;

				if (FAILED(pBufferData->pIndexBuffer->Unlock())) return;

				pBufferData->pLocalIndexBuffer = nullptr;

				this->_pDevice->DrawIndexedPrimitive(type, 0u, 0u, pBufferData->curOffset, 0u, primitiveCount);
				pBufferData->curOffset = 0u;

				return;
			}

		}

	}

}