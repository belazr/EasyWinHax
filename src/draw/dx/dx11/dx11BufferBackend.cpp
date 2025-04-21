#include "dx11BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx11 {

			BufferBackend::BufferBackend() : _pDevice{}, _pContext{}, _pPixelShader{}, _pVertexBuffer{}, _pIndexBuffer{}, _topology{}, _curTopology{}, _capacity{} {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11PixelShader* pPixelShader, D3D11_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pContext = pContext;
				this->_pPixelShader = pPixelShader;
				this->_topology = topology;

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				this->_pVertexBuffer = nullptr;
				this->_pIndexBuffer = nullptr;

				D3D11_BUFFER_DESC vertexBufferDesc{};
				vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				vertexBufferDesc.ByteWidth = capacity * sizeof(Vertex);
				vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&vertexBufferDesc, nullptr, &this->_pVertexBuffer))) return false;

				D3D11_BUFFER_DESC indexBufferDesc{};
				indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDesc.ByteWidth = capacity * sizeof(uint32_t);
				indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				if (FAILED(this->_pDevice->CreateBuffer(&indexBufferDesc, nullptr, &this->_pIndexBuffer))) {
					this->destroy();
					
					return false;
				}

				this->_capacity = capacity;

				return true;
			}


			void BufferBackend::destroy() {

				if (this->_pIndexBuffer) {
					this->_pContext->Unmap(this->_pIndexBuffer, 0u);
					this->_pIndexBuffer->Release();
					this->_pIndexBuffer = nullptr;
				}

				if (this->_pVertexBuffer) {
					this->_pContext->Unmap(this->_pVertexBuffer, 0u);
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				this->_capacity = 0u;

				return;
			}


			uint32_t BufferBackend::capacity() {

				return this->_capacity;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				D3D11_MAPPED_SUBRESOURCE subresourceVertex{};

				if (FAILED(this->_pContext->Map(this->_pVertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourceVertex))) return false;

				*ppLocalVertexBuffer = reinterpret_cast<Vertex*>(subresourceVertex.pData);

				D3D11_MAPPED_SUBRESOURCE subresourceIndex{};

				if (FAILED(this->_pContext->Map(this->_pIndexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourceIndex))) {
					this->_pContext->Unmap(this->_pVertexBuffer, 0u);

					return false;
				}

				*ppLocalIndexBuffer = reinterpret_cast<uint32_t*>(subresourceIndex.pData);

				return true;
			}


			void BufferBackend::unmap() {
				this->_pContext->Unmap(this->_pIndexBuffer, 0u);
				this->_pContext->Unmap(this->_pVertexBuffer, 0u);
				
				return;
			}


			bool BufferBackend::begin() {
				this->_pContext->IAGetPrimitiveTopology(&this->_curTopology);
				this->_pContext->IASetPrimitiveTopology(this->_topology);
				this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0u);

				constexpr UINT STRIDE = sizeof(Vertex);
				constexpr UINT OFFSET = 0u;
				this->_pContext->IASetVertexBuffers(0u, 1u, &this->_pVertexBuffer, &STRIDE, &OFFSET);
				this->_pContext->IASetIndexBuffer(this->_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) const {

				if (textureId) {
					this->_pContext->PSSetShaderResources(0u, 1u, reinterpret_cast<ID3D11ShaderResourceView**>(&textureId));
				}

				this->_pContext->DrawIndexed(count, index, 0u);

				return;
			}


			void BufferBackend::end() const {
				this->_pContext->IASetPrimitiveTopology(this->_curTopology);
				
				return;
			}

		}

	}

}