#include "dx10BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx10 {

			BufferBackend::BufferBackend() : _pDevice{}, _pPixelShader{}, _pVertexBuffer{}, _pIndexBuffer{}, _topology{} {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(ID3D10Device* pDevice, ID3D10PixelShader* pPixelShader, D3D10_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pPixelShader = pPixelShader;
				this->_topology = topology;

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				this->_pVertexBuffer = nullptr;
				this->_pIndexBuffer = nullptr;

				D3D10_BUFFER_DESC vertexBufferDesc{};
				vertexBufferDesc.BindFlags = D3D10_BIND_VERTEX_BUFFER;
				vertexBufferDesc.ByteWidth = capacity * sizeof(Vertex);
				vertexBufferDesc.Usage = D3D10_USAGE_DYNAMIC;
				vertexBufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&vertexBufferDesc, nullptr, &this->_pVertexBuffer))) return false;

				D3D10_BUFFER_DESC indexBufferDesc{};
				indexBufferDesc.BindFlags = D3D10_BIND_INDEX_BUFFER;
				indexBufferDesc.ByteWidth = capacity * sizeof(uint32_t);
				indexBufferDesc.Usage = D3D10_USAGE_DYNAMIC;
				indexBufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&indexBufferDesc, nullptr, &this->_pIndexBuffer))) return false;

				return true;
			}


			void BufferBackend::destroy() {

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

				return;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				if (FAILED(this->_pVertexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0u, reinterpret_cast<void**>(ppLocalVertexBuffer)))) return false;

				if (FAILED(this->_pIndexBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0u, reinterpret_cast<void**>(ppLocalIndexBuffer)))) {
					this->_pVertexBuffer->Unmap();
					
					return false;
				}

				return true;
			}


			bool BufferBackend::prepare() {
				this->_pVertexBuffer->Unmap();
				this->_pIndexBuffer->Unmap();

				this->_pDevice->IASetPrimitiveTopology(this->_topology);
				this->_pDevice->PSSetShader(this->_pPixelShader);
				
				constexpr UINT STRIDE = sizeof(Vertex);
				constexpr UINT OFFSET = 0u;
				this->_pDevice->IASetVertexBuffers(0u, 1u, &this->_pVertexBuffer, &STRIDE, &OFFSET);
				this->_pDevice->IASetIndexBuffer(this->_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) {

				if (textureId) {
					this->_pDevice->PSSetShaderResources(0u, 1u, reinterpret_cast<ID3D10ShaderResourceView**>(&textureId));
				}

				this->_pDevice->DrawIndexed(count, index, 0u);

				return;
			}

		}

	}

}