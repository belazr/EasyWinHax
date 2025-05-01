#include "dx12BufferBackend.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			BufferBackend::BufferBackend() : _pDevice{}, _pCommandList{}, _pPipelineState{}, _pVertexBufferResource{}, _pIndexBufferResource{}, _topology{}, _capacity{} {}


			BufferBackend::~BufferBackend() {
				this->destroy();

				return;
			}


			void BufferBackend::initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineState, D3D_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pCommandList = pCommandList;
				this->_pPipelineState = pPipelineState;
				this->_topology = topology;

				return;
			}


			bool BufferBackend::create(uint32_t capacity) {
				
				if (this->_capacity) return false;

				const uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(&this->_pVertexBufferResource, vertexBufferSize)) {
					this->destroy();

					return false;
				}

				const uint32_t indexBufferSize = capacity * sizeof(uint32_t);

				if (!this->createBuffer(&this->_pIndexBufferResource, indexBufferSize)) {
					this->destroy();
					
					return false;
				}

				this->_capacity = capacity;

				return true;
			}


			void BufferBackend::destroy() {

				if (this->_pIndexBufferResource) {
					this->_pIndexBufferResource->Unmap(0u, nullptr);
					this->_pIndexBufferResource->Release();
					this->_pIndexBufferResource = nullptr;
				}

				if (this->_pVertexBufferResource) {
					this->_pVertexBufferResource->Unmap(0u, nullptr);
					this->_pVertexBufferResource->Release();
					this->_pVertexBufferResource = nullptr;
				}

				this->_capacity = 0u;

				return;
			}


			uint32_t BufferBackend::capacity() const {

				return this->_capacity;
			}


			bool BufferBackend::map(Vertex** ppLocalVertexBuffer, uint32_t** ppLocalIndexBuffer) {

				if (FAILED(this->_pVertexBufferResource->Map(0u, nullptr, reinterpret_cast<void**>(ppLocalVertexBuffer)))) {
					this->unmap();

					return false;
				}

				if (FAILED(this->_pIndexBufferResource->Map(0u, nullptr, reinterpret_cast<void**>(ppLocalIndexBuffer)))) {
					this->unmap();

					return false;
				}

				return true;
			}


			void BufferBackend::unmap() {
				this->_pIndexBufferResource->Unmap(0u, nullptr);
				this->_pVertexBufferResource->Unmap(0u, nullptr);

				return;
			}


			bool BufferBackend::begin() {
				this->_pCommandList->IASetPrimitiveTopology(this->_topology);
				this->_pCommandList->SetPipelineState(this->_pPipelineState);

				D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
				vertexBufferView.BufferLocation = this->_pVertexBufferResource->GetGPUVirtualAddress();
				vertexBufferView.SizeInBytes = this->_capacity * sizeof(Vertex);
				vertexBufferView.StrideInBytes = sizeof(Vertex);
				this->_pCommandList->IASetVertexBuffers(0u, 1u, &vertexBufferView);

				D3D12_INDEX_BUFFER_VIEW indexBufferView{};
				indexBufferView.BufferLocation = this->_pIndexBufferResource->GetGPUVirtualAddress();
				indexBufferView.SizeInBytes = this->_capacity * sizeof(uint32_t);
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				this->_pCommandList->IASetIndexBuffer(&indexBufferView);

				return true;
			}


			void BufferBackend::draw(TextureId textureId, uint32_t index, uint32_t count) const {
				
				if (textureId) {
					const D3D12_GPU_DESCRIPTOR_HANDLE hTextureDesc{ textureId };
					this->_pCommandList->SetGraphicsRootDescriptorTable(1u, hTextureDesc);
				}
				
				this->_pCommandList->DrawIndexedInstanced(count, 1u, index, 0, 0u);

				return;
			}


			void BufferBackend::end() const {

				return;
			}


			bool BufferBackend::createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const {
				D3D12_HEAP_PROPERTIES heapProperties{};
				heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

				D3D12_RESOURCE_DESC resourceDesc{};
				resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				resourceDesc.Width = size;
				resourceDesc.Height = 1u;
				resourceDesc.DepthOrArraySize = 1u;
				resourceDesc.MipLevels = 1u;
				resourceDesc.SampleDesc.Count = 1u;
				resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

				return SUCCEEDED(this->_pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(ppBufferResource)));
			}

		}

	}

}