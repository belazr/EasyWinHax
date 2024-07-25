#include "dx9DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx9 {

			DrawBuffer::DrawBuffer() : _pDevice{}, _pVertexBuffer {}, _pIndexBuffer{}, _primitiveType{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE primitiveType) {
				this->_pDevice = pDevice;
				this->_primitiveType = primitiveType;

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				this->reset();

				this->_pVertexBuffer = nullptr;
				this->_pIndexBuffer = nullptr;

				constexpr DWORD D3DFVF_CUSTOM = D3DFVF_XYZ | D3DFVF_DIFFUSE;

				const UINT vertexBufferSize = vertexCount * sizeof(Vertex);

				if (FAILED(this->_pDevice->CreateVertexBuffer(vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOM, D3DPOOL_DEFAULT, &this->_pVertexBuffer, nullptr))) return false;

				this->_vertexBufferSize = vertexBufferSize;

				const UINT indexBufferSize = vertexCount * sizeof(uint32_t);

				if (FAILED(this->_pDevice->CreateIndexBuffer(indexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &this->_pIndexBuffer, nullptr))) return false;

				this->_indexBufferSize = indexBufferSize;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_pVertexBuffer) {
					this->_pVertexBuffer->Unlock();
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				if (this->_pIndexBuffer) {
					this->_pIndexBuffer->Unlock();
					this->_pIndexBuffer->Release();
					this->_pIndexBuffer = nullptr;
				}

				this->reset();

				return;
			}


			bool DrawBuffer::map() {

				if (!this->_pLocalVertexBuffer) {

					if (FAILED(this->_pVertexBuffer->Lock(0u, this->_vertexBufferSize, reinterpret_cast<void**>(&this->_pLocalVertexBuffer), D3DLOCK_DISCARD))) return false;

				}

				if (!this->_pLocalIndexBuffer) {

					if (FAILED(this->_pIndexBuffer->Lock(0u, this->_indexBufferSize, reinterpret_cast<void**>(&this->_pLocalIndexBuffer), D3DLOCK_DISCARD))) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				UINT primitiveCount{};

				switch (this->_primitiveType) {
				case D3DPRIMITIVETYPE::D3DPT_POINTLIST:
					primitiveCount = this->_curOffset;
					break;
				case D3DPRIMITIVETYPE::D3DPT_LINELIST:
					primitiveCount = this->_curOffset / 2;
					break;
				case D3DPRIMITIVETYPE::D3DPT_LINESTRIP:
					primitiveCount = this->_curOffset - 1;
					break;
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST:
					primitiveCount = this->_curOffset / 3;
					break;
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLESTRIP:
				case D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN:
					primitiveCount = this->_curOffset - 2;
					break;
				default:
					return;
				}

				if (FAILED(this->_pDevice->SetStreamSource(0u, this->_pVertexBuffer, 0u, sizeof(Vertex)))) return;

				if (FAILED(this->_pDevice->SetIndices(this->_pIndexBuffer))) return;

				if (FAILED(this->_pVertexBuffer->Unlock())) return;

				this->_pLocalVertexBuffer = nullptr;

				if (FAILED(this->_pIndexBuffer->Unlock())) return;

				this->_pLocalIndexBuffer = nullptr;

				this->_pDevice->DrawIndexedPrimitive(this->_primitiveType, 0u, 0u, this->_curOffset, 0u, primitiveCount);
				
				this->_curOffset = 0u;

				return;
			}


		}

	}

}