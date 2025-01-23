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

		}

	}

}