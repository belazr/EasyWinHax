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

        }

    }

}