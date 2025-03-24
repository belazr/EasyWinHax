#include "dx10DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx10 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(), _pDevice{}, _topology{}, _pPixelShaderPassthrough{}, _pPixelShaderTexture{}, _pVertexBuffer {}, _pIndexBuffer{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(ID3D10Device* pDevice, D3D10_PRIMITIVE_TOPOLOGY topology, ID3D10PixelShader* pPixelShaderPassthrough, ID3D10PixelShader* pPixelShaderTexture) {
				this->_pDevice = pDevice;
				this->_topology = topology;
				this->_pPixelShaderPassthrough = pPixelShaderPassthrough;
				this->_pPixelShaderTexture = pPixelShaderTexture;

				return;
			}


			bool DrawBuffer::create(uint32_t capacity) {
				this->reset();

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

				this->_pTextureBuffer = reinterpret_cast<TextureId*>(malloc(capacity * sizeof(TextureId)));

				if (!this->_pTextureBuffer) return false;

				this->_capacity = capacity;

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


			void DrawBuffer::draw() {
				D3D10_PRIMITIVE_TOPOLOGY curTopology{};
				this->_pDevice->IAGetPrimitiveTopology(&curTopology);

				this->_pVertexBuffer->Unmap();
				this->_pLocalVertexBuffer = nullptr;

				this->_pIndexBuffer->Unmap();
				this->_pLocalIndexBuffer = nullptr;

				constexpr UINT STRIDE = sizeof(Vertex);
				constexpr UINT OFFSET = 0u;
				this->_pDevice->IASetVertexBuffers(0u, 1u, &this->_pVertexBuffer, &STRIDE, &OFFSET);
				this->_pDevice->IASetIndexBuffer(this->_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0u);
				this->_pDevice->IASetPrimitiveTopology(this->_topology);

				uint32_t drawCount = 1u;

				for (uint32_t i = 0u; i < this->_size; i += drawCount) {
					drawCount = 1u;

					ID3D10ShaderResourceView* const pCurTextureView = reinterpret_cast<ID3D10ShaderResourceView*>(static_cast<uintptr_t>(this->_pTextureBuffer[i]));

					for (uint32_t j = i + 1u; j < this->_size; j++) {
						ID3D10ShaderResourceView* const pNextTextureView = reinterpret_cast<ID3D10ShaderResourceView*>(static_cast<uintptr_t>(this->_pTextureBuffer[j]));

						if (pNextTextureView != pCurTextureView) break;

						drawCount++;
					}

					ID3D10PixelShader* const pPixelShader = pCurTextureView ? this->_pPixelShaderTexture : this->_pPixelShaderPassthrough;
					this->_pDevice->PSSetShader(pPixelShader);
					this->_pDevice->PSSetShaderResources(0u, 1u, &pCurTextureView);
					this->_pDevice->DrawIndexed(drawCount, i, 0u);
				}


				this->_pDevice->IASetPrimitiveTopology(curTopology);

				this->_size = 0u;

				return;
			}

		}

	}

}