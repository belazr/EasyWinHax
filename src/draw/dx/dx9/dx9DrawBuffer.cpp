#include "dx9DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx9 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(), _pDevice{}, _primitiveType{}, _pPixelShaderPassthrough{}, _pPixelShaderTexture{},
				_pVertexBuffer{}, _pIndexBuffer{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}


			void DrawBuffer::initialize(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE primitiveType, IDirect3DPixelShader9* pPixelShaderPassthrough, IDirect3DPixelShader9* pPixelShaderTexture) {
				this->_pDevice = pDevice;
				this->_primitiveType = primitiveType;
				this->_pPixelShaderPassthrough = pPixelShaderPassthrough;
				this->_pPixelShaderTexture = pPixelShaderTexture;

				return;
			}


			bool DrawBuffer::create(uint32_t capacity) {
				this->reset();

				this->_pVertexBuffer = nullptr;
				this->_pIndexBuffer = nullptr;
				this->_pTextureBuffer = nullptr;

				const UINT vertexBufferSize = capacity * sizeof(Vertex);

				if (FAILED(this->_pDevice->CreateVertexBuffer(vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0u, D3DPOOL_DEFAULT, &this->_pVertexBuffer, nullptr))) return false;

				const UINT indexBufferSize = capacity * sizeof(uint32_t);

				if (FAILED(this->_pDevice->CreateIndexBuffer(indexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &this->_pIndexBuffer, nullptr))) return false;

				this->_pTextureBuffer = reinterpret_cast<TextureId*>(malloc(capacity * sizeof(TextureId)));

				if (!this->_pTextureBuffer) return false;

				this->_capacity = capacity;

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

				if (this->_pTextureBuffer) {
					free(this->_pTextureBuffer);
					this->_pTextureBuffer = nullptr;
				}

				this->reset();

				return;
			}


			bool DrawBuffer::map() {

				if (!this->_pLocalVertexBuffer) {

					if (FAILED(this->_pVertexBuffer->Lock(0u, this->_capacity * sizeof(Vertex), reinterpret_cast<void**>(&this->_pLocalVertexBuffer), D3DLOCK_DISCARD))) return false;

				}

				if (!this->_pLocalIndexBuffer) {

					if (FAILED(this->_pIndexBuffer->Lock(0u, this->_capacity * sizeof(uint32_t), reinterpret_cast<void**>(&this->_pLocalIndexBuffer), D3DLOCK_DISCARD))) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {

				if (FAILED(this->_pVertexBuffer->Unlock())) return;

				this->_pLocalVertexBuffer = nullptr;

				if (FAILED(this->_pIndexBuffer->Unlock())) return;

				this->_pLocalIndexBuffer = nullptr;

				if (FAILED(this->_pDevice->SetStreamSource(0u, this->_pVertexBuffer, 0u, sizeof(Vertex)))) return;

				if (FAILED(this->_pDevice->SetIndices(this->_pIndexBuffer))) return;

				uint32_t drawCount = 1u;

				for (uint32_t i = 0u; i < this->_size; i += drawCount) {
					drawCount = 1u;

					IDirect3DTexture9* const pCurTexture = reinterpret_cast<IDirect3DTexture9*>(static_cast<uintptr_t>(this->_pTextureBuffer[i]));

					for (uint32_t j = i + 1u; j < this->_size; j++) {
						IDirect3DTexture9* const pNextTexture = reinterpret_cast<IDirect3DTexture9*>(static_cast<uintptr_t>(this->_pTextureBuffer[j]));

						if (pNextTexture != pCurTexture) break;
						
						drawCount++;
					}

					IDirect3DPixelShader9* const pPixelShader = pCurTexture ? this->_pPixelShaderTexture : this->_pPixelShaderPassthrough;

					if (FAILED(this->_pDevice->SetPixelShader(pPixelShader))) continue;

					if (FAILED(this->_pDevice->SetTexture(0u, pCurTexture))) continue;

					UINT primitiveCount = 0u;

					switch (this->_primitiveType) {
					case D3DPRIMITIVETYPE::D3DPT_POINTLIST:
						primitiveCount = drawCount;
						break;
					case D3DPRIMITIVETYPE::D3DPT_LINELIST:
						primitiveCount = drawCount / 2;
						break;
					case D3DPRIMITIVETYPE::D3DPT_LINESTRIP:
						primitiveCount = drawCount - 1;
						break;
					case D3DPRIMITIVETYPE::D3DPT_TRIANGLELIST:
						primitiveCount = drawCount / 3;
						break;
					case D3DPRIMITIVETYPE::D3DPT_TRIANGLESTRIP:
					case D3DPRIMITIVETYPE::D3DPT_TRIANGLEFAN:
						primitiveCount = drawCount - 2;
						break;
					default:
						return;
					}

					this->_pDevice->DrawIndexedPrimitive(this->_primitiveType, 0u, 0u, drawCount, i, primitiveCount);
				}

				this->_size = 0u;

				return;
			}


		}

	}

}