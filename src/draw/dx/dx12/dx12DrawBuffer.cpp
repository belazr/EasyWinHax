#include "dx12DrawBuffer.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			DrawBuffer::DrawBuffer() : AbstractDrawBuffer(),
				_pDevice{}, _pCommandList{}, _pPipelineStatePassthrough{}, _pPipelineStateTexture{},
				_topology {}, _pVertexBufferResource{}, _pIndexBufferResource{} {}


			DrawBuffer::~DrawBuffer() {
				this->destroy();

				return;
			}

			void DrawBuffer::initialize(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineStatePassthrough, ID3D12PipelineState* pPipelineStateTexture, D3D_PRIMITIVE_TOPOLOGY topology) {
				this->_pDevice = pDevice;
				this->_pCommandList = pCommandList;
				this->_pPipelineStatePassthrough = pPipelineStatePassthrough;
				this->_pPipelineStateTexture = pPipelineStateTexture;
				this->_topology = topology;

				return;
			}


			bool DrawBuffer::create(uint32_t capacity) {
				this->reset();

				this->_pVertexBufferResource = nullptr;
				this->_pIndexBufferResource = nullptr;

				const uint32_t vertexBufferSize = capacity * sizeof(Vertex);

				if (!this->createBuffer(&this->_pVertexBufferResource, vertexBufferSize)) return false;

				const uint32_t indexBufferSize = capacity * sizeof(uint32_t);

				if (!this->createBuffer(&this->_pIndexBufferResource, indexBufferSize)) return false;

				this->_pTextureBuffer = reinterpret_cast<TextureId*>(malloc(capacity * sizeof(TextureId)));

				if (!this->_pTextureBuffer) return false;

				this->_capacity = capacity;

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

				if (this->_pTextureBuffer) {
					free(this->_pTextureBuffer);
					this->_pTextureBuffer = nullptr;
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
				vertexBufferView.SizeInBytes = this->_size * sizeof(Vertex);
				vertexBufferView.StrideInBytes = sizeof(Vertex);
				this->_pCommandList->IASetVertexBuffers(0u, 1u, &vertexBufferView);
				
				D3D12_INDEX_BUFFER_VIEW indexBufferView{};
				indexBufferView.BufferLocation = this->_pIndexBufferResource->GetGPUVirtualAddress();
				indexBufferView.SizeInBytes = this->_size * sizeof(uint32_t);
				indexBufferView.Format = DXGI_FORMAT_R32_UINT;
				this->_pCommandList->IASetIndexBuffer(&indexBufferView);
				
				this->_pCommandList->IASetPrimitiveTopology(this->_topology);

				uint32_t drawCount = 1u;

				for (uint32_t i = 0u; i < this->_size; i += drawCount) {
					drawCount = 1u;

					const D3D12_GPU_DESCRIPTOR_HANDLE hCurTextureDesc{ this->_pTextureBuffer[i] };

					for (uint32_t j = i + 1u; j < this->_size; j++) {
						const D3D12_GPU_DESCRIPTOR_HANDLE hNextTextureDesc{ this->_pTextureBuffer[j] };

						if (hNextTextureDesc.ptr != hCurTextureDesc.ptr) break;

						drawCount++;
					}

					ID3D12PipelineState* const pPipleState = hCurTextureDesc.ptr ? this->_pPipelineStateTexture : this->_pPipelineStatePassthrough;
					this->_pCommandList->SetPipelineState(pPipleState);
					this->_pCommandList->SetGraphicsRootDescriptorTable(1u, hCurTextureDesc);
					this->_pCommandList->DrawIndexedInstanced(drawCount, 1u, i, 0, 0u);
				}

				this->_size = 0u;

				return;
			}


			bool DrawBuffer::createBuffer(ID3D12Resource** ppBufferResource, uint32_t size) const {
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