#include "dx12Backend.h"
#include "dx12Shaders.h"
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

            bool getInitData(InitData* pInitData) {
                IDXGIFactory4* pDxgiFactory = nullptr;

                if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pDxgiFactory)))) return false;
                
                ID3D12Device* pDevice = nullptr;

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

                IDXGISwapChain1* pSwapChain1 = nullptr;
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

                IDXGISwapChain3* pSwapChain3 = nullptr;

                if (FAILED(pSwapChain1->QueryInterface(IID_PPV_ARGS(&pSwapChain3)))) {
                    pSwapChain1->Release();

                    return nullptr;
                }

                pSwapChain1->Release();

                return pSwapChain3;
            }


            Backend::Backend() :
                _pSwapChain{}, _pCommandQueue{}, _hMainWindow{}, _pDevice{}, _pTextureCommandAllocator{}, _pTextureCommandList{}, _pCommandList{},
                _pRtvDescriptorHeap{}, _hRtvHeapStartDescriptor{}, _pSrvDescriptorHeap{}, _hSrvHeapStartCpuDescriptor{}, _hSrvHeapStartGpuDescriptor{}, _srvHeapDescriptorIncrementSize{},
                _pRootSignature{}, _pTriangleListPipelineStatePassthrough {}, _pPointListPipelineStatePassthrough{}, _pTriangleListPipelineStateTexture{},
				_pFence{}, _viewport{}, _pRtvResource{}, _frameDataVector{}, _curBackBufferIndex{}, _pCurFrameData{}, _textures{} {}


            Backend::~Backend() {

                if (this->_pRtvResource) {
                    this->_pRtvResource->Release();
                }

                if (this->_pFence) {
                    this->_pFence->Release();
                }

                if (this->_pTriangleListPipelineStateTexture) {
                    this->_pTriangleListPipelineStateTexture->Release();
                }

                if (this->_pPointListPipelineStatePassthrough) {
                    this->_pPointListPipelineStatePassthrough->Release();
                }

				if (this->_pTriangleListPipelineStatePassthrough) {
					this->_pTriangleListPipelineStatePassthrough->Release();
				}

                if (this->_pRootSignature) {
                    this->_pRootSignature->Release();
                }

                if (this->_pSrvDescriptorHeap) {
                    this->_pSrvDescriptorHeap->Release();
                }

                if (this->_pRtvDescriptorHeap) {
                    this->_pRtvDescriptorHeap->Release();
                }

                if (this->_pCommandList) {
                    this->_pCommandList->Release();
                }

                if (this->_pTextureCommandList) {
                    this->_pTextureCommandList->Release();
                }

                if (this->_pTextureCommandAllocator) {
                    this->_pTextureCommandAllocator->Release();
                }

                for (size_t i = 0u; i < this->_textures.size(); i++) {
                    this->_textures[i]->Release();
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

                if (!this->_pTextureCommandAllocator) {
                    
                    if (FAILED(this->_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->_pTextureCommandAllocator)))) return false;
                
                }

                if (!this->_pTextureCommandList) {
                    this->_pTextureCommandList = this->createCommandList();
                }

                if (!this->_pTextureCommandList) return false;

                if (!this->_pCommandList) {
                    this->_pCommandList = this->createCommandList();
                }

                if (!this->_pCommandList) return false;

                if (!this->createDescriptorHeaps()) return false;

                if (!this->_pRootSignature) {

                    if (!this->createRootSignature()) return false;

                }

                if (!this->_pTriangleListPipelineStatePassthrough) {
                    
					this->_pTriangleListPipelineStatePassthrough = this->createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, { PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH) });
                    
                }

                if (!this->_pTriangleListPipelineStatePassthrough) return false;


                if (!this->_pPointListPipelineStatePassthrough) {

                    this->_pPointListPipelineStatePassthrough = this->createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT, { PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH) });

                }

                if (!this->_pPointListPipelineStatePassthrough) return false;

                if (!this->_pTriangleListPipelineStateTexture) {

                    this->_pTriangleListPipelineStateTexture = this->createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, { PIXEL_SHADER_TEXTURE, sizeof(PIXEL_SHADER_TEXTURE) });

                }

                if (!this->_pTriangleListPipelineStateTexture) return false;

                if (!this->_pFence) {
                    
                    if (FAILED(this->_pDevice->CreateFence(UINT64_MAX, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->_pFence)))) return false;

                }

                return true;
            }


            TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
                D3D12_HEAP_PROPERTIES heapProps{};
                heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

                D3D12_RESOURCE_DESC resDesc{};
                resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                resDesc.Alignment = 0u;
                resDesc.Width = width;
                resDesc.Height = height;
                resDesc.DepthOrArraySize = 1u;
                resDesc.MipLevels = 1u;
                resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                resDesc.SampleDesc.Count = 1u;
                resDesc.SampleDesc.Quality = 0u;

                ID3D12Resource* pTexture = nullptr;
                
                if (FAILED(this->_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pTexture)))) return 0ull;

                const uint32_t pitch = (width * sizeof(Color) + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
                const UINT uploadSize = height * pitch;
                
                heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

                resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
                resDesc.Width = uploadSize;
                resDesc.Height = 1u;
                resDesc.Format = DXGI_FORMAT_UNKNOWN;
                resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

                ID3D12Resource* pBuffer = nullptr;

                if (FAILED(this->_pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pBuffer)))) {
                    pTexture->Release();

                    return 0ull;
                }

                BYTE* pLocalBuffer = nullptr;
                const D3D12_RANGE range{ 0u, uploadSize };
                
                if (FAILED(pBuffer->Map(0u, &range, reinterpret_cast<void**>(&pLocalBuffer)))) {
                    pBuffer->Release();
                    pTexture->Release();
                    
                    return 0ull;
                }

                for (uint32_t i = 0u; i < height; i++) {
                    memcpy(pLocalBuffer + i * pitch, reinterpret_cast<const BYTE*>(data) + i * width * sizeof(Color), width * sizeof(Color));
                }

                pBuffer->Unmap(0u, &range);

                if (!this->uploadTexture(pTexture, pBuffer, width, height, pitch)) {
                    pBuffer->Release();
                    pTexture->Release();

                    return 0ull;
                }

                pBuffer->Release();

                const D3D12_CPU_DESCRIPTOR_HANDLE hSrvHeapCpuDescriptor{
                    this->_hSrvHeapStartCpuDescriptor.ptr + this->_srvHeapDescriptorIncrementSize * this->_textures.size()
                };

                D3D12_SHADER_RESOURCE_VIEW_DESC shaderResDesc{};
                shaderResDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                shaderResDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                shaderResDesc.Texture2D.MipLevels = resDesc.MipLevels;
                shaderResDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                this->_pDevice->CreateShaderResourceView(pTexture, &shaderResDesc, hSrvHeapCpuDescriptor);
                
                const D3D12_GPU_DESCRIPTOR_HANDLE hSrvHeapGpuDescriptor{
                    this->_hSrvHeapStartGpuDescriptor.ptr + this->_srvHeapDescriptorIncrementSize * this->_textures.size() 
                };
                
                this->_textures.append(pTexture);

                return hSrvHeapGpuDescriptor.ptr;
            }


            bool Backend::beginFrame() {
                DXGI_SWAP_CHAIN_DESC swapchainDesc{};

                if (FAILED(this->_pSwapChain->GetDesc(&swapchainDesc))) return false;

                if (this->_frameDataVector.size() != swapchainDesc.BufferCount) {

                    if (!this->resizeFrameDataVector(swapchainDesc.BufferCount)) return false;
                
                }

                this->_curBackBufferIndex = this->_pSwapChain->GetCurrentBackBufferIndex();
                this->_pCurFrameData = this->_frameDataVector + this->_curBackBufferIndex;

                if (WaitForSingleObject(this->_pCurFrameData->hEvent, INFINITE) != WAIT_OBJECT_0) return false;

                if (!this->getCurrentViewport(&this->_viewport)) return false;
                                
                if (FAILED(this->_pCurFrameData->pCommandAllocator->Reset())) return false;

                if (FAILED(this->_pCommandList->Reset(this->_pCurFrameData->pCommandAllocator, nullptr))) return false;

                if (!this->createRenderTargetView(swapchainDesc.BufferDesc.Format)) return false;

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
                    this->_pFence->SetEventOnCompletion(static_cast<UINT64>(this->_curBackBufferIndex), this->_pCurFrameData->hEvent);
                    this->_pCommandQueue->ExecuteCommandLists(1u, reinterpret_cast<ID3D12CommandList**>(&this->_pCommandList));
                    this->_pCommandQueue->Signal(this->_pFence, static_cast<UINT64>(this->_curBackBufferIndex));
                }

                this->_pRtvResource->Release();
                this->_pRtvResource = nullptr;

                return;
            }


            IBufferBackend* Backend::getTriangleListBufferBackend() {

                return &this->_pCurFrameData->triangleListBuffer;
            }


            IBufferBackend* Backend::getPointListBufferBackend() {

                return &this->_pCurFrameData->pointListBuffer;
            }


            IBufferBackend* Backend::getTextureTriangleListBufferBackend() {

                return &this->_pCurFrameData->textureTriangleListBuffer;
            }


            void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
                *frameWidth = this->_viewport.Width;
                *frameHeight = this->_viewport.Height;

                return;
            }


            ID3D12GraphicsCommandList* Backend::createCommandList() const {
                ID3D12GraphicsCommandList* pCommandList = nullptr;

                if (FAILED(this->_pDevice->CreateCommandList(0u, D3D12_COMMAND_LIST_TYPE_DIRECT, this->_pTextureCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList)))) return nullptr;

                if (FAILED(pCommandList->Close())) {
                    pCommandList->Release();

                    return nullptr;
                }

                return pCommandList;
            }


            bool Backend::createDescriptorHeaps() {
                
                if (!this->_pRtvDescriptorHeap) {
                    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
                    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                    descriptorHeapDesc.NumDescriptors = 1u;
                    descriptorHeapDesc.NodeMask = 1u;

                    if (FAILED(this->_pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&this->_pRtvDescriptorHeap)))) return false;

                }

                this->_hRtvHeapStartDescriptor = this->_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                if (!this->_hRtvHeapStartDescriptor.ptr) return false;

                if (!this->_pSrvDescriptorHeap) {
                    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
                    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
                    // 1000 textures should be enough, otherwise bad luck
                    descriptorHeapDesc.NumDescriptors = 1000u;
                    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

                    if (FAILED(this->_pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&this->_pSrvDescriptorHeap)))) return false;

                }
                
                this->_hSrvHeapStartCpuDescriptor = this->_pSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

                if (!this->_hSrvHeapStartCpuDescriptor.ptr) return false;

                this->_hSrvHeapStartGpuDescriptor = this->_pSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

                if (!this->_hSrvHeapStartGpuDescriptor.ptr) return false;

                this->_srvHeapDescriptorIncrementSize = this->_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

                return true;
            }


            bool Backend::createRootSignature() {
                D3D12_DESCRIPTOR_RANGE descRange {};
                descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                descRange.NumDescriptors = 1u;

                D3D12_ROOT_PARAMETER parameter[2]{};
                parameter[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                parameter[0].Constants.ShaderRegister = 0u;
                parameter[0].Constants.RegisterSpace = 0u;
                parameter[0].Constants.Num32BitValues = 16u;
                parameter[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

                parameter[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                parameter[1].DescriptorTable.NumDescriptorRanges = 1u;
                parameter[1].DescriptorTable.pDescriptorRanges = &descRange;
                parameter[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

                D3D12_STATIC_SAMPLER_DESC staticSampler{};
                staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                staticSampler.MipLODBias = 0.f;
                staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
                staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                staticSampler.MinLOD = 0.f;
                staticSampler.MaxLOD = 0.f;
                staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

                D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
                rootSignatureDesc.NumParameters = _countof(parameter);
                rootSignatureDesc.pParameters = parameter;
                rootSignatureDesc.NumStaticSamplers = 1u;
                rootSignatureDesc.pStaticSamplers = &staticSampler;
                rootSignatureDesc.Flags =
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

                ID3DBlob* pSigantureBlob = nullptr;
                
                if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigantureBlob, nullptr))) return false;

                if (FAILED(this->_pDevice->CreateRootSignature(0u, pSigantureBlob->GetBufferPointer(), pSigantureBlob->GetBufferSize(), IID_PPV_ARGS(&this->_pRootSignature)))) {
                    pSigantureBlob->Release();

                    return false;
                }
                
                pSigantureBlob->Release();

                return true;
            }
            
            
            ID3D12PipelineState* Backend::createPipelineState(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology, D3D12_SHADER_BYTECODE pixelShader) const {
                DXGI_SWAP_CHAIN_DESC swapchainDesc{};

                if (FAILED(this->_pSwapChain->GetDesc(&swapchainDesc))) return nullptr;

                constexpr D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[]{
                    { "POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u },
                    { "COLOR",    0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u},
                    { "TEXCOORD",    0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0u}
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
                pipelineStateDesc.PS = pixelShader;
                pipelineStateDesc.PrimitiveTopologyType = topology;
                pipelineStateDesc.pRootSignature = this->_pRootSignature;
                pipelineStateDesc.SampleMask = UINT_MAX;
                pipelineStateDesc.NumRenderTargets = 1u;
                pipelineStateDesc.RTVFormats[0] = swapchainDesc.BufferDesc.Format;
                pipelineStateDesc.SampleDesc.Count = 1u;
                pipelineStateDesc.InputLayout = { INPUT_LAYOUT, _countof(INPUT_LAYOUT)};
                pipelineStateDesc.BlendState = blendDesc;
                pipelineStateDesc.RasterizerState = rasterizerDesc;
                pipelineStateDesc.DepthStencilState = depthStencilDesc;

                ID3D12PipelineState* pPipelineState = nullptr;

                if (FAILED(this->_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pPipelineState)))) return nullptr;
                
                return pPipelineState;
            }


            bool Backend::uploadTexture(ID3D12Resource* pTexture, ID3D12Resource* pBuffer, uint32_t width, uint32_t height, uint32_t pitch) const {
                
                if (FAILED(this->_pTextureCommandAllocator->Reset())) return 0ull;

                if (FAILED(this->_pTextureCommandList->Reset(this->_pTextureCommandAllocator, nullptr))) return 0ull;

                D3D12_TEXTURE_COPY_LOCATION srcLocation{};
                srcLocation.pResource = pBuffer;
                srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                srcLocation.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srcLocation.PlacedFootprint.Footprint.Width = width;
                srcLocation.PlacedFootprint.Footprint.Height = height;
                srcLocation.PlacedFootprint.Footprint.Depth = 1u;
                srcLocation.PlacedFootprint.Footprint.RowPitch = pitch;

                D3D12_TEXTURE_COPY_LOCATION dstLocation{};
                dstLocation.pResource = pTexture;
                dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

                this->_pTextureCommandList->CopyTextureRegion(&dstLocation, 0u, 0u, 0u, &srcLocation, nullptr);

                D3D12_RESOURCE_BARRIER barrier{};
                barrier.Transition.pResource = pTexture;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

                this->_pTextureCommandList->ResourceBarrier(1u, &barrier);

                if (FAILED(this->_pTextureCommandList->Close())) return false;

                ID3D12Fence* pFence = nullptr;

                if (FAILED(this->_pDevice->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)))) return false;

                const HANDLE hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);

                if (hEvent == nullptr) return false;

                if (FAILED(pFence->SetEventOnCompletion(1u, hEvent))) {
                    CloseHandle(hEvent);
                    pFence->Release();

                    return false;
                }

                this->_pCommandQueue->ExecuteCommandLists(1u, reinterpret_cast<ID3D12CommandList* const *>(&this->_pTextureCommandList));

                if (FAILED(this->_pCommandQueue->Signal(pFence, 1u))) {
                    CloseHandle(hEvent);
                    pFence->Release();

                    return false;
                }

                WaitForSingleObject(hEvent, INFINITE);
                CloseHandle(hEvent);
                pFence->Release();
                
                return true;
            }


            bool Backend::resizeFrameDataVector(UINT size) {
                // destroy old frame data to be safe
                this->_frameDataVector.resize(0u);
                this->_frameDataVector.resize(size);

                for (UINT i = 0u; i < size; i++) {
                    
                    if (!this->_frameDataVector[i].create(
                        this->_pDevice, this->_pCommandList, this->_pTriangleListPipelineStatePassthrough,
                        this->_pPointListPipelineStatePassthrough, this->_pTriangleListPipelineStateTexture
                    )) {
                        this->_frameDataVector.resize(0u);

                        return false;
                    }

                }

                return true;
            }


            bool Backend::createRenderTargetView(DXGI_FORMAT format) {
                D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc{};
                renderTargetViewDesc.Format = format;
                renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

                if (FAILED(this->_pSwapChain->GetBuffer(this->_curBackBufferIndex, IID_PPV_ARGS(&this->_pRtvResource)))) return false;

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