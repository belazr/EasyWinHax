#include "dx12Backend.h"

namespace hax {

    namespace draw {

        namespace dx12 {

            bool getDXGISwapChain3VTable(void** pSwapChainVTable, size_t size) {
                IDXGIFactory4* pDxgiFactory = nullptr;

                if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)))) return false;
                
                ID3D12Device* pDevice = nullptr;

                D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;

                if (FAILED(D3D12CreateDevice(nullptr, featureLevel, IID_PPV_ARGS(&pDevice)))) {
                    pDxgiFactory->Release();
                    
                    return false;
                }

                ID3D12CommandQueue* pCommandQueue = nullptr;

                D3D12_COMMAND_QUEUE_DESC queueDesc{};
                
                if (FAILED(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)))) {
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }
                    
                IDXGISwapChain1* pSwapChain1 = nullptr;

                DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
                swapChainDesc.BufferCount = 3;
                swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.SampleDesc.Count = 1;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                
                const BOOL consoleAllocated = AllocConsole();

                if (FAILED(pDxgiFactory->CreateSwapChainForHwnd(pCommandQueue, GetConsoleWindow(), &swapChainDesc, nullptr, nullptr, &pSwapChain1))) {
                    
                    if (consoleAllocated) {
                        FreeConsole();
                    }
                    
                    pCommandQueue->Release();
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }

                if (consoleAllocated) {
                    FreeConsole();
                }
                    
                IDXGISwapChain3* pSwapChain3 = nullptr;

                if (FAILED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain3)))) {
                    pSwapChain1->Release();
                    pCommandQueue->Release();
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }
                    
                if (memcpy_s(pSwapChainVTable, size, *reinterpret_cast<void**>(pSwapChain3), size)) {
                    pSwapChain3->Release();
                    pSwapChain1->Release();
                    pCommandQueue->Release();
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }

                pSwapChain3->Release();
                pSwapChain1->Release();
                pCommandQueue->Release();
                pDevice->Release();
                pDxgiFactory->Release();

                return true;
            }


            Backend::Backend() :
                _pSwapChain{}, _pDevice{}, _pCommandQueue{}, _pFence{}, _pRtvDescriptorHeap{},
                _pImageDataArray{}, _imageCount{} {}


            Backend::~Backend() {

                this->destroyImageDataArray();

                if (this->_pRtvDescriptorHeap) {
                    this->_pRtvDescriptorHeap->Release();
                }

                if (this->_pFence) {
                    this->_pFence->Release();
                }

                if (this->_pCommandQueue) {
                    this->_pCommandQueue->Release();
                }

                if (this->_pDevice) {
                    this->_pDevice->Release();
                }

            }


            void Backend::setHookArguments(void* pArg1, void* pArg2) {
                UNREFERENCED_PARAMETER(pArg2);

                this->_pSwapChain = reinterpret_cast<IDXGISwapChain3*>(pArg1);

                return;
            }


            bool Backend::initialize() {

                if (!this->_pDevice) {

                    if (FAILED(this->_pSwapChain->GetDevice(IID_PPV_ARGS(&this->_pDevice)))) return false;

                }

                if (!this->_pCommandQueue) {
                    D3D12_COMMAND_QUEUE_DESC queueDesc{};

                    if (FAILED(this->_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->_pCommandQueue)))) return false;

                }

                if (!this->_pFence) {

                    if (FAILED(this->_pDevice->CreateFence(0ull, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->_pFence)))) return false;

                }

                if (!this->_pRtvDescriptorHeap) {
                    DXGI_SWAP_CHAIN_DESC swapchainDesc{};

                    if (FAILED(this->_pSwapChain->GetDesc(&swapchainDesc))) return false;

                    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
                    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                    descriptorHeapDesc.NumDescriptors = swapchainDesc.BufferCount;
                    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                    descriptorHeapDesc.NodeMask = 1;

                    if (FAILED(this->_pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&this->_pRtvDescriptorHeap)))) return false;

                }

                return true;
            }


            bool Backend::beginFrame() {
                DXGI_SWAP_CHAIN_DESC swapchainDesc{};

                if (FAILED(this->_pSwapChain->GetDesc(&swapchainDesc))) return false;

                if (!this->resizeImageDataArray(swapchainDesc.BufferCount)) {
                    this->destroyImageDataArray();

                    return false;
                }

                return true;
            }



            void Backend::endFrame() {

                return;
            }


            AbstractDrawBuffer* Backend::getTriangleListBuffer() {

                return nullptr;
            }


            AbstractDrawBuffer* Backend::getPointListBuffer() {

                return nullptr;
            }


            void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {

                return;
            }


            bool Backend::resizeImageDataArray(uint32_t imageCount) {

                if (imageCount == this->_imageCount) return true;

                if (imageCount < this->_imageCount) {

                    for (uint32_t i = this->_imageCount; i >= imageCount; i--) {
                        this->destroyImageData(&this->_pImageDataArray[i]);
                    }

                    this->_imageCount = imageCount;

                    return true;
                }

                ImageData* const pOldImageData = this->_pImageDataArray;
                uint32_t oldImageCount = this->_imageCount;

                this->_pImageDataArray = new ImageData[imageCount]{};
                this->_imageCount = imageCount;

                const UINT rtvHandleIncrement = this->_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

                if (!rtvHandleIncrement) return false;

                const D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = this->_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                if (!rtvHandle.ptr) return false;

                D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
                renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

                for (uint32_t i = 0; i < this->_imageCount; i++) {

                    if (pOldImageData && i < oldImageCount) {
                        memcpy(&this->_pImageDataArray[i], &pOldImageData[i], sizeof(ImageData));
                    }
                    else {

                        if (FAILED(this->_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&this->_pImageDataArray[i].pRenderTargetResource)))) return false;
                        
                        this->_pImageDataArray[i].hRenderTargetDescriptor = { rtvHandle.ptr + i * rtvHandleIncrement };

                        this->_pDevice->CreateRenderTargetView(this->_pImageDataArray[i].pRenderTargetResource, &renderTargetViewDesc, this->_pImageDataArray[i].hRenderTargetDescriptor);

                        if (FAILED(this->_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->_pImageDataArray[i].pCommandAllocator)))) return false;

                    }

                }

                if (pOldImageData) {
                    delete[] pOldImageData;
                }

                return true;
            }


            void Backend::destroyImageDataArray() {

                if (!this->_pImageDataArray) return;

                for (uint32_t i = 0u; i < this->_imageCount; i++) {
                    this->destroyImageData(&this->_pImageDataArray[i]);
                }

                delete[] this->_pImageDataArray;
                this->_pImageDataArray = nullptr;
                this->_imageCount = 0u;

                return;
            }


            void Backend::destroyImageData(ImageData* pImageData) const {

                if (pImageData->pCommandAllocator) {
                    pImageData->pCommandAllocator->Release();
                    pImageData->pCommandAllocator = nullptr;
                }

                if (pImageData->pRenderTargetResource) {
                    pImageData->pRenderTargetResource->Release();
                    pImageData->pRenderTargetResource = nullptr;
                }

            }

        }

    }

}