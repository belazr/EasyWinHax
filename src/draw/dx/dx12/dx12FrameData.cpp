#include "dx12FrameData.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			FrameData::FrameData() : pCommandAllocator{}, triangleListBuffer{}, pointListBuffer{}, textureTriangleListBuffer{}, hEvent{} {}


			FrameData::FrameData(FrameData&& fd) noexcept :
				pCommandAllocator{ fd.pCommandAllocator }, triangleListBuffer{ static_cast<BufferBackend&&>(fd.triangleListBuffer) }, pointListBuffer{ static_cast<BufferBackend&&>(fd.pointListBuffer) },
				textureTriangleListBuffer{ static_cast<BufferBackend&&>(fd.textureTriangleListBuffer) }, hEvent{ fd.hEvent } {
				fd.pCommandAllocator = nullptr;
				fd.hEvent = nullptr;
			};


			FrameData::~FrameData() {
				this->destroy();

				return;
			}


			bool FrameData::create(
				ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pTriangleListPipelineStatePassthrough,
				ID3D12PipelineState* pPointListPipelineStatePassthrough, ID3D12PipelineState* pTriangleListPipelineStateTexture
			) {
				if (FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->pCommandAllocator)))) {
					this->destroy();

					return false;
				}

				constexpr size_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 100u;
				constexpr size_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;

				this->triangleListBuffer.initialize(pDevice, pCommandList, pTriangleListPipelineStatePassthrough, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				if (!this->triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) {
					this->destroy();

					return false;
				}

				this->pointListBuffer.initialize(pDevice, pCommandList, pPointListPipelineStatePassthrough, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

				if (!this->pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) {
					this->destroy();

					return false;
				}

				this->textureTriangleListBuffer.initialize(pDevice, pCommandList, pTriangleListPipelineStateTexture, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				if (!this->textureTriangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) {
					this->destroy();

					return false;
				}

				this->hEvent = CreateEventA(nullptr, FALSE, TRUE, nullptr);

				if (!this->hEvent) {
					this->destroy();

					return false;
				}

				return true;
			}


			void FrameData::destroy() {

				if (this->hEvent) {
					WaitForSingleObject(this->hEvent, INFINITE);
					CloseHandle(this->hEvent);
					this->hEvent = nullptr;
				}

				this->textureTriangleListBuffer.destroy();
				this->pointListBuffer.destroy();
				this->triangleListBuffer.destroy();

				if (this->pCommandAllocator) {
					this->pCommandAllocator->Release();
					this->pCommandAllocator = nullptr;
				}

				return;
			}

		}

	}

}