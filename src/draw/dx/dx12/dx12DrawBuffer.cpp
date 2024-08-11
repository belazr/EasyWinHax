#include "dx12DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			DrawBuffer::DrawBuffer() : _pDevice{}, _pCommandList{}, _pPipelineState{}, _topology{}, _pVertexBufferResource {}, _pIndexBufferResource{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}

			void DrawBuffer::initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineState, D3D_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pCommandList = pCommandList;
				this->_pPipelineState = pPipelineState;
				this->_topology = topology;

				return;
			}


			bool DrawBuffer::create(uint32_t vertexCount) {
				this->reset();

				this->_pVertexBufferResource = nullptr;
				this->_pIndexBufferResource = nullptr;

				const uint32_t vertexBufferSize = vertexCount * sizeof(Vertex);

				if (!this->createBuffer(&this->_pVertexBufferResource, vertexBufferSize)) return false;

				this->_vertexBufferSize = vertexBufferSize;

				const uint32_t indexBufferSize = vertexCount * sizeof(uint32_t);

				if (!this->createBuffer(&this->_pIndexBufferResource, indexBufferSize)) return false;

				this->_indexBufferSize = indexBufferSize;

				return true;
			}


			void DrawBuffer::destroy() {

				if (this->_pVertexBufferResource) {
					this->_pVertexBufferResource->Release();
					this->_pVertexBufferResource = nullptr;
				}

				if (this->_pIndexBufferResource) {
					this->_pIndexBufferResource->Release();
					this->_pIndexBufferResource = nullptr;
				}

				this->reset();

				return;
			}


			bool DrawBuffer::map() {

				if (!this->_pLocalVertexBuffer) {
					
					if (FAILED(this->_pVertexBufferResource->Map(0u, nullptr, reinterpret_cast<void**>(&this->_pLocalVertexBuffer)))) return false;

				}
				
				if (!this->_pLocalIndexBuffer) {

					if (FAILED(this->_pIndexBufferResource->Map(0u, nullptr, reinterpret_cast<void**>(&this->_pLocalIndexBuffer)))) return false;

				}

				return true;
			}


			void DrawBuffer::draw() {
				this->_pVertexBufferResource->Unmap(0u, nullptr);
				this->_pLocalVertexBuffer = nullptr;

				this->_pIndexBufferResource->Unmap(0u, nullptr);
				this->_pLocalIndexBuffer = nullptr;

				D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
				vertexBufferView.BufferLocation = this->_pVertexBufferResource->GetGPUVirtualAddress();
				vertexBufferView.SizeInBytes = this->_vertexBufferSize;
				vertexBufferView.StrideInBytes = sizeof(Vertex);
				this->_pCommandList->IASetVertexBuffers(0u, 1u, &vertexBufferView);
				
				D3D12_INDEX_BUFFER_VIEW indexBufferView{};
				indexBufferView.BufferLocation = this->_pIndexBufferResource->GetGPUVirtualAddress();
				indexBufferView.SizeInBytes = this->_indexBufferSize;
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				this->_pCommandList->IASetIndexBuffer(&indexBufferView);
				
				this->_pCommandList->IASetPrimitiveTopology(this->_topology);
				this->_pCommandList->SetPipelineState(this->_pPipelineState);
				this->_pCommandList->DrawIndexedInstanced(this->_curOffset, 1u, 0u, 0u, 0u);

				this->_curOffset = 0u;

				return;
			}


			bool DrawBuffer::createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const {
				D3D12_HEAP_PROPERTIES heapProperties{};
				heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
				heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
				heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

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