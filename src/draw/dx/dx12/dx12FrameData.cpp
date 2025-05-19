#include "dx12FrameData.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			FrameData::FrameData() : pCommandAllocator{}, textureBufferBackend{}, solidBufferBackend{}, hEvent{} {}


			FrameData::FrameData(FrameData&& fd) noexcept :
				pCommandAllocator{ fd.pCommandAllocator }, textureBufferBackend{ static_cast<BufferBackend&&>(fd.textureBufferBackend) },
				solidBufferBackend{ static_cast<BufferBackend&&>(fd.solidBufferBackend) }, hEvent{ fd.hEvent } {
				fd.pCommandAllocator = nullptr;
				fd.hEvent = nullptr;
			};


			FrameData::~FrameData() {
				this->destroy();

				return;
			}


			bool FrameData::create(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPipelineStateTexture, ID3D12PipelineState* pPipelineStatePassthrough) {
				
				if (FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->pCommandAllocator)))) {
					this->destroy();

					return false;
				}

				constexpr size_t INITIAL_BUFFER_SIZE = 100u;

				this->textureBufferBackend.initialize(pDevice, pCommandList, pPipelineStateTexture);

				if (!this->textureBufferBackend.create(INITIAL_BUFFER_SIZE)) {
					this->destroy();

					return false;
				}

				this->solidBufferBackend.initialize(pDevice, pCommandList, pPipelineStatePassthrough);

				if (!this->solidBufferBackend.create(INITIAL_BUFFER_SIZE)) {
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

				this->solidBufferBackend.destroy();
				this->textureBufferBackend.destroy();

				if (this->pCommandAllocator) {
					this->pCommandAllocator->Release();
					this->pCommandAllocator = nullptr;
				}

				return;
			}

		}

	}

}