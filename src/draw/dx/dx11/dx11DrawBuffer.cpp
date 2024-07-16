#include "dx11DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx11 {

			DrawBuffer::DrawBuffer() : _pDevice{}, _pContext{}, _pVertexBuffer{}, _pIndexBuffer{}, _topology{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, D3D11_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pContext = pContext;
				this->_topology = topology;

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				memset(this, 0, sizeof(DrawBuffer));

				D3D11_BUFFER_DESC vertexBufferDesc{};
				vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				vertexBufferDesc.ByteWidth = vertexCount * sizeof(Vertex);
				vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&vertexBufferDesc, nullptr, &this->_pVertexBuffer))) return false;

				this->vertexBufferSize = vertexBufferDesc.ByteWidth;

				D3D11_BUFFER_DESC indexBufferDesc{};
				indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDesc.ByteWidth = vertexCount * sizeof(uint32_t);
				indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&indexBufferDesc, nullptr, &this->_pIndexBuffer))) return false;

				this->indexBufferSize = indexBufferDesc.ByteWidth;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_pVertexBuffer) {
					this->_pContext->Unmap(this->_pVertexBuffer, 0u);
					this->pLocalVertexBuffer = nullptr;
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				if (this->_pIndexBuffer) {
					this->_pContext->Unmap(this->_pIndexBuffer, 0u);
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
					D3D11_MAPPED_SUBRESOURCE subresourcePoints{};

					if (FAILED(this->_pContext->Map(this->_pVertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourcePoints))) return false;

					this->pLocalVertexBuffer = reinterpret_cast<Vertex*>(subresourcePoints.pData);
				}

				if (!this->pLocalIndexBuffer) {
					D3D11_MAPPED_SUBRESOURCE subresourcePoints{};

					if (FAILED(this->_pContext->Map(this->_pIndexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourcePoints))) return false;

					this->pLocalIndexBuffer = reinterpret_cast<uint32_t*>(subresourcePoints.pData);
				}

				return true;
			}


			void DrawBuffer::draw() {
				D3D11_PRIMITIVE_TOPOLOGY curTopology{};
				this->_pContext->IAGetPrimitiveTopology(&curTopology);

				if (!curTopology) return;

				this->_pContext->Unmap(this->_pVertexBuffer, 0u);
				this->pLocalVertexBuffer = nullptr;

				this->_pContext->Unmap(this->_pIndexBuffer, 0u);
				this->pLocalIndexBuffer = nullptr;

				constexpr UINT STRIDE = sizeof(Vertex);
				constexpr UINT OFFSET = 0u;
				this->_pContext->IASetVertexBuffers(0u, 1u, &this->_pVertexBuffer, &STRIDE, &OFFSET);
				this->_pContext->IASetIndexBuffer(this->_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);
				this->_pContext->IASetPrimitiveTopology(this->_topology);
				this->_pContext->DrawIndexed(this->curOffset, 0u, 0u);
				this->_pContext->IASetPrimitiveTopology(curTopology);

				this->curOffset = 0u;

				return;
			}

		}

	}

}