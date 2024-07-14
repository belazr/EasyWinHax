#include "dx9DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx9 {

			DrawBuffer::DrawBuffer(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE primitiveType) : _pDevice{ pDevice }, _pVertexBuffer {}, _pIndexBuffer{}, _primitiveType{ primitiveType } {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				memset(this, 0, sizeof(DrawBuffer));

				constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZ | D3DFVF_DIFFUSE;
				const UINT newVertexBufferSize = vertexCount * sizeof(Vertex);

				if (FAILED(this->_pDevice->CreateVertexBuffer(newVertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &this->_pVertexBuffer, nullptr))) return false;

				this->vertexBufferSize = newVertexBufferSize;

				const UINT newIndexBufferSize = vertexCount * sizeof(uint32_t);

				if (FAILED(this->_pDevice->CreateIndexBuffer(newIndexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &this->_pIndexBuffer, nullptr))) return false;

				this->indexBufferSize = newIndexBufferSize;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_pVertexBuffer) {
					this->_pVertexBuffer->Unlock();
					this->pLocalVertexBuffer = nullptr;
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				if (this->_pIndexBuffer) {
					this->_pIndexBuffer->Unlock();
					this->pLocalIndexBuffer = nullptr;
					this->_pIndexBuffer->Release();
					this->_pIndexBuffer = nullptr;
				}

				this->vertexBufferSize = 0u;
				this->indexBufferSize = 0u;
				this->curOffset = 0u;

				return;
			}


			bool DrawBuffer::map() {

				if (!this->pLocalVertexBuffer) {

					if (FAILED(this->_pVertexBuffer->Lock(0u, this->vertexBufferSize, reinterpret_cast<void**>(&this->pLocalVertexBuffer), D3DLOCK_DISCARD))) return false;

				}

				if (!this->pLocalIndexBuffer) {

					if (FAILED(this->_pIndexBuffer->Lock(0u, this->indexBufferSize, reinterpret_cast<void**>(&this->pLocalIndexBuffer), D3DLOCK_DISCARD))) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				UINT primitiveCount{};

				switch (this->_primitiveType) {
				case D3DPRIMITIVETYPE::D3DPT_POINTLIST:
					primitiveCount = this->curOffset;
					break;
				case D3DPRIMITIVETYPE::D3DPT_LINELIST:
					primitiveCount = this->curOffset / 2;
					break;
				case D3DPRIMITIVETYPE::D3DPT_LINESTRIP:
					primitiveCount = this->curOffset - 1;
					break;
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST:
					primitiveCount = this->curOffset / 3;
					break;
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLESTRIP:
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN:
					primitiveCount = this->curOffset - 2;
					break;
				default:
					return;
				}

				if (FAILED(this->_pDevice->SetStreamSource(0u, this->_pVertexBuffer, 0u, sizeof(Vertex)))) return;

				if (FAILED(this->_pDevice->SetIndices(this->_pIndexBuffer))) return;

				if (FAILED(this->_pVertexBuffer->Unlock())) return;

				this->pLocalVertexBuffer = nullptr;

				if (FAILED(this->_pIndexBuffer->Unlock())) return;

				this->pLocalIndexBuffer = nullptr;

				this->_pDevice->DrawIndexedPrimitive(this->_primitiveType, 0u, 0u, this->curOffset, 0u, primitiveCount);
				
				this->curOffset = 0u;

				return;
			}


		}

	}

}