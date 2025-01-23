#include "dx10DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx10 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(), _pDevice{}, _topology{}, _pVertexBuffer{}, _pIndexBuffer{} {}


			void DrawBuffer::initialize(ID3D10Device* pDevice, D3D10_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_topology = topology;

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				this->reset();

				this->_pVertexBuffer = nullptr;
				this->_pIndexBuffer = nullptr;

				D3D10_BUFFER_DESC vertexBufferDesc{};
				vertexBufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
				vertexBufferDesc.ByteWidth = vertexCount * sizeof(Vertex);
				vertexBufferDesc.Usage = D3D10_USAGE_DYNAMIC;
				vertexBufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&vertexBufferDesc, nullptr, &this->_pVertexBuffer))) return false;

				this->_vertexBufferSize = vertexBufferDesc.ByteWidth;

				D3D10_BUFFER_DESC indexBufferDesc{};
				indexBufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
				indexBufferDesc.ByteWidth = vertexCount * sizeof(uint32_t);
				indexBufferDesc.Usage = D3D10_USAGE_DYNAMIC;
				indexBufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&indexBufferDesc, nullptr, &this->_pIndexBuffer))) return false;

				this->_indexBufferSize = indexBufferDesc.ByteWidth;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_pVertexBuffer) {
					this->_pVertexBuffer->Unmap();
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				if (this->_pIndexBuffer) {
					this->_pIndexBuffer->Unmap();
					this->_pIndexBuffer->Release();
					this->_pIndexBuffer = nullptr;
				}

				this->reset();

				return;
			}


			bool DrawBuffer::map() {

				if (!this->_pLocalVertexBuffer) {

					if (FAILED(this->_pVertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0u, reinterpret_cast<void**>(&this->_pLocalVertexBuffer)))) return false;

				}

				if (!this->_pLocalIndexBuffer) {

					if (FAILED(this->_pIndexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0u, reinterpret_cast<void**>(&this->_pLocalIndexBuffer)))) return false;

				}

				return true;
			}

		}

	}

}