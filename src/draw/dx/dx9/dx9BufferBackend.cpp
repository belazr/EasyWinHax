#include "dx9BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx9 {

			BufferBackend::BufferBackend() : _pDevice{}, _pPixelShader{}, _pVertexBuffer{}, _pIndexBuffer{}, _capacity{} {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(IDirect3DDevice9* pDevice, IDirect3DPixelShader9* pPixelShader) {
				this->_pDevice = pDevice;
				this->_pPixelShader = pPixelShader;

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				
				if (this->_capacity) return false;
				
				const UINT vertexBufferSize = capacity * sizeof(Vertex);

				if (FAILED(this->_pDevice->CreateVertexBuffer(vertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0u, D3DPOOL_DEFAULT, &this->_pVertexBuffer, nullptr))) {
					this->destroy();
					
					return false;
				}

				const UINT indexBufferSize = capacity * sizeof(uint32_t);

				if (FAILED(this->_pDevice->CreateIndexBuffer(indexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &this->_pIndexBuffer, nullptr))) {
					this->destroy();

					return false;
				}

				this->_capacity = capacity;

				return true;
			}


			void BufferBackend::destroy() {

				if (this->_pIndexBuffer) {
					this->_pIndexBuffer->Unlock();
					this->_pIndexBuffer->Release();
					this->_pIndexBuffer = nullptr;
				}

				if (this->_pVertexBuffer) {
					this->_pVertexBuffer->Unlock();
					this->_pVertexBuffer->Release();
					this->_pVertexBuffer = nullptr;
				}

				this->_capacity = 0u;

				return;
			}


			uint32_t BufferBackend::capacity() const {
				
				return this->_capacity;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				if (FAILED(this->_pVertexBuffer->Lock(0u, this->_capacity * sizeof(Vertex), reinterpret_cast<void**>(ppLocalVertexBuffer), D3DLOCK_DISCARD))) {
					this->unmap();

					return false;
				}
				
				if (FAILED(this->_pIndexBuffer->Lock(0u, this->_capacity * sizeof(uint32_t), reinterpret_cast<void**>(ppLocalIndexBuffer), D3DLOCK_DISCARD))) {
					this->unmap();

					return false;
				}

				return true;
			}

			void BufferBackend::unmap() {
				this->_pIndexBuffer->Unlock();
				this->_pVertexBuffer->Unlock();
				
				return;
			}


			bool BufferBackend::prepare() {
				
				if (FAILED(this->_pDevice->SetPixelShader(this->_pPixelShader))) return false;

				if (FAILED(this->_pDevice->SetStreamSource(0u, this->_pVertexBuffer, 0u, sizeof(Vertex)))) return false;

				if (FAILED(this->_pDevice->SetIndices(this->_pIndexBuffer))) return false;
			
				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) const {

				if (textureId) {
					if (FAILED(this->_pDevice->SetTexture(0u, reinterpret_cast<IDirect3DTexture9*>(static_cast<uintptr_t>(textureId))))) return;
				}

				const UINT primitiveCount = count / 3u;
				this->_pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0u, 0u, count, index, primitiveCount);

				return;
			}

		}

	}

}