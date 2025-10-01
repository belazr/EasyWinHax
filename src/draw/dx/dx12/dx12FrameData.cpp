#include "dx12FrameData.h"

namespace hax {

	namespace draw {

		namespace dx12 {

			FrameData::FrameData() : pCommandAllocator{}, bufferBackend{}, hEvent{} {}


			FrameData::~FrameData() {
				this->destroy();

				return;
			}


			bool FrameData::create(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCommandList) {
				
				if (FAILED(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->pCommandAllocator)))) {
					this->destroy();

					return false;
				}

				constexpr size_t INITIAL_BUFFER_SIZE = 100u;

				this->bufferBackend.initialize(pDevice, pCommandList);

				if (!this->bufferBackend.create(INITIAL_BUFFER_SIZE)) {
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

				this->bufferBackend.destroy();

				if (this->pCommandAllocator) {
					this->pCommandAllocator->Release();
					this->pCommandAllocator = nullptr;
				}

				return;
			}

		}

	}

}