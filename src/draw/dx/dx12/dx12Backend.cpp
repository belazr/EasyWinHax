#include "dx12Backend.h"
#include "..\..\..\hooks\TrampHook.h"
#include "..\..\..\mem.h"

namespace hax {

    namespace draw {

        namespace dx12 {

            static hax::in::TrampHook* pExecuteHook;
            static HANDLE hHookSemaphore;
            static ID3D12CommandQueue* pHookCommandQueue;

            typedef void(WINAPI* tExecuteCommandLists)(ID3D12CommandQueue* pCommandQueue, UINT numCommandLists, ID3D12CommandList* ppCommandLists);

            static void WINAPI hkExecuteCommandLists(ID3D12CommandQueue* pCommandQueue, UINT numCommandLists, ID3D12CommandList* ppCommandLists) {

                if (pCommandQueue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
                    pHookCommandQueue = pCommandQueue;
                    pExecuteHook->disable();
                    const tExecuteCommandLists pExecuteCommandLists = reinterpret_cast<tExecuteCommandLists>(pExecuteHook->getOrigin());
                    ReleaseSemaphore(hHookSemaphore, 1l, nullptr);

                    return pExecuteCommandLists(pCommandQueue, numCommandLists, ppCommandLists);
                }

                return reinterpret_cast<tExecuteCommandLists>(pExecuteHook->getGateway())(pCommandQueue, numCommandLists, ppCommandLists);
            }

            static IDXGISwapChain3* createDummySwapChain3(IDXGIFactory4* pDxgiFactory, ID3D12CommandQueue* pCommandQueue);

            bool getDx12InitData(Dx12InitData* pInitData) {
                IDXGIFactory4* pDxgiFactory{};

                if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)))) return false;
                
                ID3D12Device* pDevice{};

                if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice)))) {
                    pDxgiFactory->Release();
                    
                    return false;
                }

                ID3D12CommandQueue* pCommandQueue{};
                D3D12_COMMAND_QUEUE_DESC queueDesc{};
                
                if (FAILED(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)))) {
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }
                    
                IDXGISwapChain3* const pSwapChain3 = createDummySwapChain3(pDxgiFactory, pCommandQueue);

                if (!pSwapChain3) {
                    pCommandQueue->Release();
                    pDevice->Release();
                    pDxgiFactory->Release();

                    return false;
                }
                    
                pInitData->pPresent = reinterpret_cast<tPresent>(hax::mem::in::getVirtualFunction(pSwapChain3, 8u));
                const tExecuteCommandLists pExecuteCommandLists = reinterpret_cast<tExecuteCommandLists>(hax::mem::in::getVirtualFunction(pCommandQueue, 10u));

                pSwapChain3->Release();
                pCommandQueue->Release();
                pDevice->Release();
                pDxgiFactory->Release();

                if (!pInitData->pPresent || !pExecuteCommandLists) return false;

                hHookSemaphore = CreateSemaphoreA(nullptr, 0l, 1l, nullptr);

                if (!hHookSemaphore) return false;

                pExecuteHook = new hax::in::TrampHook(reinterpret_cast<BYTE*>(pExecuteCommandLists), reinterpret_cast<BYTE*>(hkExecuteCommandLists), 0x5u);
                
                if (!pExecuteHook->enable()) {
                    delete pExecuteHook;
                    CloseHandle(hHookSemaphore);

                    return false;
                }

                WaitForSingleObject(hHookSemaphore, INFINITE);
                CloseHandle(hHookSemaphore);
                delete pExecuteHook;

                pInitData->pCommandQueue = pHookCommandQueue;

                return pInitData->pCommandQueue != nullptr;
            }


            static IDXGISwapChain3* createDummySwapChain3(IDXGIFactory4* pDxgiFactory, ID3D12CommandQueue* pCommandQueue) {
                const BOOL consoleAllocated = AllocConsole();

                IDXGISwapChain1* pSwapChain1{};
                DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
                swapChainDesc.BufferCount = 3u;
                swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
                swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDesc.SampleDesc.Count = 1u;
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

                if (FAILED(pDxgiFactory->CreateSwapChainForHwnd(pCommandQueue, GetConsoleWindow(), &swapChainDesc, nullptr, nullptr, &pSwapChain1))) {

                    if (consoleAllocated) {
                        FreeConsole();
                    }

                    return nullptr;
                }

                if (consoleAllocated) {
                    FreeConsole();
                }

                IDXGISwapChain3* pSwapChain3{};

                if (FAILED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain3)))) {
                    pSwapChain1->Release();

                    return nullptr;
                }

                pSwapChain1->Release();

                return pSwapChain3;
            }


            Backend::Backend() :
                _pSwapChain{}, _pCommandQueue{}, _hMainWindow{}, _pDevice{},
                _pRtvDescriptorHeap{}, _hRtvHeapStartDescriptor{}, _pRootSignature{}, _pPipelineState{}, _pFence{},
                _pCommandList{}, _viewport{}, _pRtvResource{},
                _pImageDataArray {}, _imageCount{}, _pCurImageData{} {}


            Backend::~Backend() {

                if (this->_pRtvResource) {
                    this->_pRtvResource->Release();
                }
                
                this->destroyImageDataArray();

                if (this->_pCommandList) {
                    this->_pCommandList->Release();
                }

                if (this->_pFence) {
                    this->_pFence->Release();
                }

                if (this->_pPipelineState) {
                    this->_pPipelineState->Release();
                }

                if (this->_pRootSignature) {
                    this->_pRootSignature->Release();
                }

                if (this->_pRtvDescriptorHeap) {
                    this->_pRtvDescriptorHeap->Release();
                }

                if (this->_pDevice) {
                    this->_pDevice->Release();
                }

            }


            void Backend::setHookArguments(void* pArg1, void* pArg2) {
                this->_pSwapChain = reinterpret_cast<IDXGISwapChain3*>(pArg1);
                this->_pCommandQueue = reinterpret_cast<ID3D12CommandQueue*>(pArg2);

                return;
            }


            static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

            bool Backend::initialize() {
                EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&this->_hMainWindow));

                if (!this->_hMainWindow) return false;

                if (!this->_pDevice) {

                    if (FAILED(this->_pSwapChain->GetDevice(IID_PPV_ARGS(&this->_pDevice)))) return false;

                }

                if (!this->_pRtvDescriptorHeap) {

                    if (!this->createDescriptorHeap()) return false;

                }

                this->_hRtvHeapStartDescriptor = this->_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                if (!this->_hRtvHeapStartDescriptor.ptr) return false;

                if (!this->_pRootSignature) {

                    if (!this->createRootSignature()) return false;

                }

                if (!this->_pPipelineState) {
                    
                    if (!this->createPipelineState()) return false;
                    
                }

                if (!this->_pFence) {
                    
                    if (FAILED(this->_pDevice->CreateFence(UINT64_MAX, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->_pFence)))) return false;

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

                const UINT backBufferIndex = this->_pSwapChain->GetCurrentBackBufferIndex();
                this->_pCurImageData = &this->_pImageDataArray[backBufferIndex];

                if (WaitForSingleObject(this->_pCurImageData->hEvent, INFINITE) != WAIT_OBJECT_0) return false;

                if (!this->getCurrentViewport(&this->_viewport)) return false;
                                
                if (FAILED(this->_pCurImageData->pCommandAllocator->Reset())) return false;

                if (FAILED(this->_pCommandList->Reset(this->_pCurImageData->pCommandAllocator, nullptr))) return false;

                if (!this->createRenderTargetView(swapchainDesc.BufferDesc.Format, backBufferIndex)) return false;

                D3D12_RESOURCE_BARRIER resourceBarrier{};
                resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrier.Transition.pResource = this->_pRtvResource;
                resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

                this->_pCommandList->ResourceBarrier(1u, &resourceBarrier);
                this->_pCommandList->OMSetRenderTargets(1u, &this->_hRtvHeapStartDescriptor, FALSE, nullptr);
                
                this->_pCommandList->RSSetViewports(1u, &this->_viewport);
                
                const float left = this->_viewport.TopLeftX;
                const float top = this->_viewport.TopLeftY;
                const float right = this->_viewport.TopLeftX + this->_viewport.Width;
                const float bottom = this->_viewport.TopLeftY + this->_viewport.Height;

                const D3D12_RECT rect{ static_cast<LONG>(left), static_cast<LONG>(top), static_cast<LONG>(right), static_cast<LONG>(bottom) };                
                this->_pCommandList->RSSetScissorRects(1u, &rect);

                this->_pCommandList->SetGraphicsRootSignature(this->_pRootSignature);

                const float ortho[]{
                    2.f / (right - left), 0.f, 0.f, 0.f,
                    0.f, 2.f / (top - bottom), 0.f, 0.f,
                    0.f, 0.f, .5f, 0.f,
                    (left + right) / (left - right), (top + bottom) / (bottom - top), .5f, 1.f
                };

                this->_pCommandList->SetGraphicsRoot32BitConstants(0u, _countof(ortho), &ortho, 0u);
                this->_pCommandList->SetPipelineState(this->_pPipelineState);

                return true;
            }



            void Backend::endFrame() {
                D3D12_RESOURCE_BARRIER resourceBarrier{};
                resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                resourceBarrier.Transition.pResource = this->_pRtvResource;
                resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

                this->_pCommandList->ResourceBarrier(1u, &resourceBarrier);
                
                if (SUCCEEDED(this->_pCommandList->Close())) {
                    this->_pFence->SetEventOnCompletion(static_cast<UINT64>(this->_pSwapChain->GetCurrentBackBufferIndex()), this->_pCurImageData->hEvent);
                    this->_pCommandQueue->ExecuteCommandLists(1u, reinterpret_cast<ID3D12CommandList**>(&this->_pCommandList));
                    this->_pCommandQueue->Signal(this->_pFence, static_cast<UINT64>(this->_pSwapChain->GetCurrentBackBufferIndex()));
                }

                this->_pRtvResource->Release();
                this->_pRtvResource = nullptr;

                return;
            }


            AbstractDrawBuffer* Backend::getTriangleListBuffer() {

                return &this->_pCurImageData->triangleListBuffer;
            }


            AbstractDrawBuffer* Backend::getPointListBuffer() {

                return &this->_pCurImageData->pointListBuffer;
            }


            void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
                *frameWidth = this->_viewport.Width;
                *frameHeight = this->_viewport.Height;

                return;
            }


            bool Backend::createDescriptorHeap() {
                D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
                descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                descriptorHeapDesc.NumDescriptors = 1u;
                descriptorHeapDesc.NodeMask = 1u;

                return SUCCEEDED(this->_pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&this->_pRtvDescriptorHeap)));
            }


            bool Backend::createRootSignature() {
                D3D12_ROOT_PARAMETER parameter{};

                parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                parameter.Constants.ShaderRegister = 0u;
                parameter.Constants.RegisterSpace = 0u;
                parameter.Constants.Num32BitValues = 16u;
                parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

                D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
                rootSignatureDesc.NumParameters = 1u;
                rootSignatureDesc.pParameters = &parameter;
                rootSignatureDesc.Flags =
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

                ID3DBlob* pSigantureBlob{};
                
                if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigantureBlob, nullptr))) return false;

                if (FAILED(this->_pDevice->CreateRootSignature(0u, pSigantureBlob->GetBufferPointer(), pSigantureBlob->GetBufferSize(), IID_PPV_ARGS(&this->_pRootSignature)))) {
                    pSigantureBlob->Release();

                    return false;
                }
                
                pSigantureBlob->Release();

                return true;
            }

            /*
            cbuffer vertexBuffer : register(b0)
            {
              float4x4 ProjectionMatrix;
            };

            struct VS_INPUT
            {
              float2 pos : POSITION;
              float4 col : COLOR0;
            };
            
            struct PS_INPUT
            {
              float4 pos : SV_POSITION;
              float4 col : COLOR0;
            };
            
            PS_INPUT main(VS_INPUT input)
            {
              PS_INPUT output;
              output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
              output.col = input.col;
              return output;
            }
            */
            static constexpr BYTE VERTEX_SHADER[]{
                0x44, 0x58, 0x42, 0x43, 0x9F, 0xD1, 0x09, 0x69, 0xC2, 0x2F, 0x52, 0x73, 0x23, 0x8F, 0x83, 0x77,
                0xD1, 0xA5, 0xA9, 0xDB, 0x01, 0x00, 0x00, 0x00, 0x70, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
                0x34, 0x00, 0x00, 0x00, 0x50, 0x01, 0x00, 0x00, 0xA0, 0x01, 0x00, 0x00, 0xF4, 0x01, 0x00, 0x00,
                0xD4, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x6C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFE, 0xFF,
                0x00, 0x01, 0x00, 0x00, 0xEC, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00,
                0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
                0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x76, 0x65, 0x72, 0x74, 0x65, 0x78, 0x42, 0x75,
                0x66, 0x66, 0x65, 0x72, 0x00, 0xAB, 0xAB, 0xAB, 0x5C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x84, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xAC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
                0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x50, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69,
                0x6F, 0x6E, 0x4D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x00, 0x66, 0x6C, 0x6F, 0x61, 0x74, 0x34, 0x78,
                0x34, 0x00, 0xAB, 0xAB, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0xBD, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
                0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65,
                0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00,
                0x49, 0x53, 0x47, 0x4E, 0x48, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
                0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
                0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0xAB,
                0x4F, 0x53, 0x47, 0x4E, 0x4C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
                0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
                0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F,
                0x52, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0xD8, 0x00, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00,
                0x36, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x59, 0x00, 0x00, 0x04, 0x46, 0x8E, 0x20, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x67, 0x00, 0x00, 0x04, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02,
                0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x08, 0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x56, 0x15, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x0A, 0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x10, 0x10, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
                0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05,
                0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };

            /*
            struct PS_INPUT
            {
              float4 pos : SV_POSITION;
              float4 col : COLOR0;
            };
            
            float4 main(PS_INPUT input) : SV_Target
            {
              return input.col;
            }
            */
            static constexpr BYTE PIXEL_SHADER[]{
                0x44, 0x58, 0x42, 0x43, 0x29, 0x32, 0x26, 0xA8, 0x0C, 0xD2, 0xDF, 0x85, 0x93, 0x8E, 0x4A, 0x0F,
                0x51, 0x32, 0xC0, 0xAE, 0x01, 0x00, 0x00, 0x00, 0x08, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
                0x34, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00,
                0x6C, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF,
                0x00, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00,
                0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
                0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
                0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65,
                0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00,
                0x49, 0x53, 0x47, 0x4E, 0x4C, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
                0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
                0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F,
                0x52, 0x00, 0xAB, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54,
                0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x3C, 0x00, 0x00, 0x00,
                0x50, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x62, 0x10, 0x00, 0x03,
                0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54,
                0x94, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            };

            bool Backend::createPipelineState() {
                DXGI_SWAP_CHAIN_DESC swapchainDesc{};

                if (FAILED(this->_pSwapChain->GetDesc(&swapchainDesc))) return false;

                constexpr D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[]{
                    { "POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
                    { "COLOR",    0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, sizeof(Vector2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u},
                };
                
                D3D12_BLEND_DESC blendDesc{};
                blendDesc.AlphaToCoverageEnable = FALSE;
                blendDesc.RenderTarget[0].BlendEnable = TRUE;
                blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
                blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
                blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
                blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

                D3D12_RASTERIZER_DESC rasterizerDesc{};
                rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
                rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
                rasterizerDesc.DepthClipEnable = TRUE;

                D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
                depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
                depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
                depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
                depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
                depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                depthStencilDesc.BackFace = depthStencilDesc.FrontFace;

                D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
                pipelineStateDesc.VS = { VERTEX_SHADER, sizeof(VERTEX_SHADER) };
                pipelineStateDesc.PS = { PIXEL_SHADER, sizeof(PIXEL_SHADER) };
                pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
                pipelineStateDesc.pRootSignature = this->_pRootSignature;
                pipelineStateDesc.SampleMask = UINT_MAX;
                pipelineStateDesc.NumRenderTargets = 1u;
                pipelineStateDesc.RTVFormats[0] = swapchainDesc.BufferDesc.Format;
                pipelineStateDesc.SampleDesc.Count = 1u;
                pipelineStateDesc.InputLayout = { INPUT_LAYOUT, 2u };
                pipelineStateDesc.BlendState = blendDesc;
                pipelineStateDesc.RasterizerState = rasterizerDesc;
                pipelineStateDesc.DepthStencilState = depthStencilDesc;

                return SUCCEEDED(this->_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&this->_pPipelineState)));
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

                for (uint32_t i = 0; i < this->_imageCount; i++) {

                    if (pOldImageData && i < oldImageCount) {
                        memcpy(&this->_pImageDataArray[i], &pOldImageData[i], sizeof(ImageData));
                    }
                    else {

                        if (FAILED(this->_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->_pImageDataArray[i].pCommandAllocator)))) return false;

                        if (!this->_pCommandList) {

                            if (FAILED(this->_pDevice->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, this->_pImageDataArray[i].pCommandAllocator, nullptr, IID_PPV_ARGS(&this->_pCommandList)))) return false;

                            if (FAILED(this->_pCommandList->Close())) return false;

                        }

                        this->_pImageDataArray[i].hEvent = CreateEventA(nullptr, FALSE, TRUE, nullptr);

                        if (!this->_pImageDataArray[i].hEvent) return false;

                        this->_pImageDataArray[i].triangleListBuffer.initialize(this->_pDevice, this->_pCommandList, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                        static constexpr size_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

                        if (!this->_pImageDataArray[i].triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

                        this->_pImageDataArray[i].pointListBuffer.initialize(this->_pDevice, this->_pCommandList, D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                        static constexpr size_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;

                        if (!this->_pImageDataArray[i].pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) return false;

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
                pImageData->pointListBuffer.destroy();
                pImageData->triangleListBuffer.destroy();

                if (pImageData->hEvent) {
                    WaitForSingleObject(pImageData->hEvent, INFINITE);
                    CloseHandle(pImageData->hEvent);
                    pImageData->hEvent = nullptr;
                }

                if (pImageData->pCommandAllocator) {
                    pImageData->pCommandAllocator->Release();
                    pImageData->pCommandAllocator = nullptr;
                }

            }


            bool Backend::createRenderTargetView(DXGI_FORMAT format, UINT backBufferIndex) {
                D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
                renderTargetViewDesc.Format = format;
                renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

                if (FAILED(this->_pSwapChain->GetBuffer(backBufferIndex, IID_PPV_ARGS(&this->_pRtvResource)))) return false;

                this->_pDevice->CreateRenderTargetView(this->_pRtvResource, &renderTargetViewDesc, this->_hRtvHeapStartDescriptor);

                return true;
            }


            bool Backend::getCurrentViewport(D3D12_VIEWPORT* pViewport) const {
                RECT clientRect{};

                if (!GetClientRect(this->_hMainWindow, &clientRect)) return false;

                pViewport->TopLeftX = static_cast<FLOAT>(clientRect.left);
                pViewport->TopLeftY = static_cast<FLOAT>(clientRect.top);
                pViewport->Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
                pViewport->Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
                pViewport->MinDepth = 0.f;
                pViewport->MaxDepth = 1.f;

                return true;
            }


            static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam) {
                DWORD processId = 0ul;
                GetWindowThreadProcessId(hWnd, &processId);

                if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;

                char className[MAX_PATH]{};

                if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

                if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

                *reinterpret_cast<HWND*>(lParam) = hWnd;

                return FALSE;
            }

        }

    }

}