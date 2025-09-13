#include "dx11Backend.h"
#include "dx11Shaders.h"
#include "..\..\..\proc.h"

namespace hax {

	namespace draw {

		namespace dx11 {

			bool getD3D11SwapChainVTable(void** pSwapChainVTable, size_t size) {
				DXGI_SWAP_CHAIN_DESC swapChainDesc{};
				swapChainDesc.BufferCount = 1u;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				swapChainDesc.OutputWindow = GetDesktopWindow();
				swapChainDesc.Windowed = TRUE;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.SampleDesc.Count = 1u;

				ID3D11Device* pDevice = nullptr;
				IDXGISwapChain* pSwapChain = nullptr;

				const HRESULT hResult = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0u, nullptr, 0u, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, nullptr, nullptr);

				if (FAILED(hResult)) return false;

				if (memcpy_s(pSwapChainVTable, size, *reinterpret_cast<void**>(pSwapChain), size)) {
					pDevice->Release();
					pSwapChain->Release();

					return false;
				}

				pDevice->Release();
				pSwapChain->Release();

				return true;
			}


			Backend::Backend() :
				_pSwapChain{}, _pDevice{}, _pContext{}, _pInputLayout{}, _pVertexShader{}, _pPixelShaderTexture{}, _pPixelShaderPassthrough{},
				_pConstantBuffer{}, _pSamplerState{}, _pBlendState{}, _viewport{}, _pRenderTargetView{}, _state{} {}


			Backend::~Backend() {
				this->releaseState();

				if (this->_pRenderTargetView) {
					this->_pRenderTargetView->Release();
				}

				if (this->_pSamplerState) {
					this->_pSamplerState->Release();
				}

				if (this->_pBlendState) {
					this->_pBlendState->Release();
				}
				
				if (this->_pConstantBuffer) {
					this->_pConstantBuffer->Release();
				}

				this->_solidBufferBackend.destroy();
				this->_textureBufferBackend.destroy();

				if (this->_pPixelShaderPassthrough) {
					this->_pPixelShaderPassthrough->Release();
				}

				if (this->_pPixelShaderTexture) {
					this->_pPixelShaderTexture->Release();
				}

				if (this->_pVertexShader) {
					this->_pVertexShader->Release();
				}

				if (this->_pInputLayout) {
					this->_pInputLayout->Release();
				}

				for (size_t i = 0; i < this->_textures.size(); i++) {
					this->_textures[i].pTextureView->Release();
					this->_textures[i].pTexture->Release();
				}

				if (this->_pContext) {
					this->_pContext->Release();
				}

				if (this->_pDevice) {
					this->_pDevice->Release();
				}

			}


			void Backend::setHookParameters(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg2);

				this->_pSwapChain = reinterpret_cast<IDXGISwapChain*>(pArg1);

				return;
			}


			bool Backend::initialize() {

				if (!this->_pDevice) {

					if (FAILED(this->_pSwapChain->GetDevice(IID_PPV_ARGS(&this->_pDevice)))) return false;
				
				}

				if (!this->_pContext) {
					this->_pDevice->GetImmediateContext(&this->_pContext);
				}

				if (!this->_pContext) return false;

				if (!this->_pInputLayout) {

					if (!this->createInputLayout()) return false;

				}

				if (!this->createShaders()) return false;

				constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;

				this->_textureBufferBackend.initialize(this->_pDevice, this->_pContext, this->_pPixelShaderTexture);

				if (!this->_textureBufferBackend.capacity()) {

					if (!this->_textureBufferBackend.create(INITIAL_BUFFER_SIZE)) return false;

				}

				this->_solidBufferBackend.initialize(this->_pDevice, this->_pContext, this->_pPixelShaderPassthrough);
				
				if (!this->_solidBufferBackend.capacity()) {

					if (!this->_solidBufferBackend.create(INITIAL_BUFFER_SIZE)) return false;

				}

				if (!this->_pConstantBuffer) {

					if (!this->createConstantBuffer()) return false;

				}

				if (!this->_pSamplerState) {

					if (!this->createSamplerState()) return false;

				}

				if (!this->_pBlendState) {

					if (!this->createBlendState()) return false;

				}

				return true;
			}


			TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				D3D11_TEXTURE2D_DESC textureDesc{};
				textureDesc.Width = width;
				textureDesc.Height = height;
				textureDesc.MipLevels = 1u;
				textureDesc.ArraySize = 1u;
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				textureDesc.SampleDesc.Count = 1u;
				textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				D3D11_SUBRESOURCE_DATA subResData = {};
				subResData.pSysMem = data;
				subResData.SysMemPitch = width * sizeof(Color);

				ID3D11Texture2D* pTexture = nullptr;

				if (FAILED(this->_pDevice->CreateTexture2D(&textureDesc, &subResData, &pTexture))) return 0ull;

				D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
				viewDesc.Format = textureDesc.Format;
				viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;

				ID3D11ShaderResourceView* pTextureView = nullptr;

				if (FAILED(this->_pDevice->CreateShaderResourceView(pTexture, &viewDesc, &pTextureView))) {
					pTexture->Release();

					return 0ull;
				}

				this->_textures.append(TextureData{ pTexture, pTextureView });

				return static_cast<TextureId>(reinterpret_cast<uintptr_t>(pTextureView));
			}


			bool Backend::beginFrame() {
				D3D11_VIEWPORT curViewport{};
				
				if(!this->getCurrentViewport(&curViewport)) return false;

				if (curViewport.Width != this->_viewport.Width || curViewport.Height != this->_viewport.Height) {
					
					if (!this->updateConstantBuffer(curViewport)) return false;

					this->_viewport = curViewport;
				}

				// the render target view is released every frame in endFrame() and there is no leftover reference to the backbuffer
				// so it has to be acquired every frame as well
				// this is done so resolution changes do not break rendering
				ID3D11Texture2D* pBackBuffer = nullptr;
				
				if (FAILED(this->_pSwapChain->GetBuffer(0u, IID_PPV_ARGS(&pBackBuffer)))) return false;

				if (FAILED(this->_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &this->_pRenderTargetView))) {
					pBackBuffer->Release();

					return false;
				}

				pBackBuffer->Release();
				
				this->saveState();

				this->_pContext->OMSetRenderTargets(1u, &this->_pRenderTargetView, nullptr);
				this->_pContext->IASetInputLayout(this->_pInputLayout);
				this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0u);
				this->_pContext->VSSetConstantBuffers(0u, 1u, &this->_pConstantBuffer);
				this->_pContext->PSSetSamplers(0u, 1u, &this->_pSamplerState);

				constexpr float BLEND_FACTOR[]{ 0.f, 0.f, 0.f, 0.f };
				this->_pContext->OMSetBlendState(this->_pBlendState, BLEND_FACTOR, 0xffffffffu);

				return true;
			}


			void Backend::endFrame() {
				this->_pRenderTargetView->Release();
				this->_pRenderTargetView = nullptr;

				this->restoreState();

				return;
			}


			IBufferBackend* Backend::getTextureBufferBackend() {

				return &this->_textureBufferBackend;
			}


			IBufferBackend* Backend::getSolidBufferBackend() {

				return &this->_solidBufferBackend;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = this->_viewport.Width;
				*frameHeight = this->_viewport.Height;

				return;
			}


			bool Backend::createInputLayout() {
				constexpr D3D11_INPUT_ELEMENT_DESC INPUT_ELEMENT_DESC[]{
					{"POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u},
					{"COLOR", 0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u},
					{"TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u}
				};

				return SUCCEEDED(this->_pDevice->CreateInputLayout(INPUT_ELEMENT_DESC, _countof(INPUT_ELEMENT_DESC), VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pInputLayout));
			}


			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(VERTEX_SHADER, sizeof(VERTEX_SHADER), nullptr, &this->_pVertexShader))) return false;

				}

				if (!this->_pPixelShaderTexture) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_TEXTURE, sizeof(PIXEL_SHADER_TEXTURE), nullptr, &this->_pPixelShaderTexture))) return false;

				}

				if (!this->_pPixelShaderPassthrough) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH), nullptr, &this->_pPixelShaderPassthrough))) return false;

				}

				return true;
			}


			bool Backend::createConstantBuffer() {
				D3D11_BUFFER_DESC bufferDesc{};
				bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
				bufferDesc.ByteWidth = 16u * sizeof(float);
				bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				return SUCCEEDED(this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &this->_pConstantBuffer));
			}


			bool Backend::createSamplerState() {
				D3D11_SAMPLER_DESC samplerDesc{};
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

				return SUCCEEDED(this->_pDevice->CreateSamplerState(&samplerDesc, &this->_pSamplerState));
			}


			bool Backend::createBlendState() {
				D3D11_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.RenderTarget[0].BlendEnable = TRUE;
				blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
				blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
				blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

				return SUCCEEDED(this->_pDevice->CreateBlendState(&blendDesc, &this->_pBlendState));
			}


			bool Backend::getCurrentViewport(D3D11_VIEWPORT* pViewport) const {
				D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
				UINT viewportCount = _countof(viewports);

				this->_pContext->RSGetViewports(&viewportCount, viewports);

				*pViewport = viewports[0];

				if (!viewportCount || !pViewport->Width) {
					HWND hMainWnd = proc::in::getMainWindowHandle();

					if (!hMainWnd) return false;

					RECT windowRect{};

					if (!GetClientRect(hMainWnd, &windowRect)) return false;

					pViewport->Width = static_cast<FLOAT>(windowRect.right);
					pViewport->Height = static_cast<FLOAT>(windowRect.bottom);
					pViewport->TopLeftX = static_cast<FLOAT>(windowRect.left);
					pViewport->TopLeftY = static_cast<FLOAT>(windowRect.top);
					pViewport->MinDepth = 0.f;
					pViewport->MaxDepth = 1.f;
					this->_pContext->RSSetViewports(1u, pViewport);
				}

				return true;
			}


			bool Backend::updateConstantBuffer(D3D11_VIEWPORT viewport) const {
				const float viewLeft = viewport.TopLeftX;
				const float viewRight = viewport.TopLeftX + viewport.Width;
				const float viewTop = viewport.TopLeftY;
				const float viewBottom = viewport.TopLeftY + viewport.Height;

				const float ortho[][4]{
					{ 2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f  },
					{ 0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f },
					{ 0.f, 0.f, .5f, 0.f },
					{ (viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f }
				};

				D3D11_MAPPED_SUBRESOURCE subresource{};

				if (FAILED(this->_pContext->Map(this->_pConstantBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresource))) return false;

				memcpy(subresource.pData, ortho, sizeof(ortho));

				this->_pContext->Unmap(this->_pConstantBuffer, 0u);

				return true;
			}


			void Backend::saveState() {
				this->_pContext->OMGetRenderTargets(1u, &this->_state.pRenderTargetView, &this->_state.pDepthStencilView);
				this->_pContext->IAGetInputLayout(&this->_state.pInputLayout);
				this->_state.vsInstancesCount = 256u;
				this->_pContext->VSGetShader(&this->_state.pVertexShader, this->_state.vsInstances, &this->_state.vsInstancesCount);
				this->_pContext->VSGetConstantBuffers(0u, 1u, &this->_state.pConstantBuffer);
				this->_pContext->PSGetSamplers(0u, 1u, &this->_state.pSamplerState);
				this->_pContext->OMGetBlendState(&this->_state.pBlendState, this->_state.blendFactor, &this->_state.sampleMask);
				this->_pContext->IAGetPrimitiveTopology(&this->_state.topology);
				this->_state.psInstancesCount = 256u;
				this->_pContext->PSGetShader(&this->_state.pPixelShader, this->_state.psInstances, &this->_state.psInstancesCount);
				this->_pContext->IAGetVertexBuffers(0u, 1u, &this->_state.pVertexBuffer, &this->_state.stride, &this->_state.offsetVtx);
				this->_pContext->IAGetIndexBuffer(&this->_state.pIndexBuffer, &this->_state.format, &this->_state.offsetIdx);
				this->_pContext->PSGetShaderResources(0u, 1u, &this->_state.pShaderResourceView);

				return;
			}


			void Backend::restoreState() {
				this->_pContext->PSSetShaderResources(0u, 1u, &this->_state.pShaderResourceView);
				this->_pContext->IASetIndexBuffer(this->_state.pIndexBuffer, this->_state.format, this->_state.offsetIdx);
				this->_pContext->IASetVertexBuffers(0u, 1u, &this->_state.pVertexBuffer, &this->_state.stride, &this->_state.offsetVtx);
				this->_pContext->PSSetShader(this->_state.pPixelShader, this->_state.psInstances, this->_state.psInstancesCount);
				this->_pContext->IASetPrimitiveTopology(this->_state.topology);
				this->_pContext->OMSetBlendState(this->_state.pBlendState, this->_state.blendFactor, this->_state.sampleMask);
				this->_pContext->PSSetSamplers(0u, 1u, &this->_state.pSamplerState);
				this->_pContext->VSSetConstantBuffers(0u, 1u, &this->_state.pConstantBuffer);
				this->_pContext->VSSetShader(this->_state.pVertexShader, this->_state.vsInstances, this->_state.vsInstancesCount);
				this->_pContext->IASetInputLayout(this->_state.pInputLayout);
				this->_pContext->OMSetRenderTargets(1u, &this->_state.pRenderTargetView, this->_state.pDepthStencilView);

				this->releaseState();

				return;
			}


			void Backend::releaseState() {

				if (this->_state.pShaderResourceView) {
					this->_state.pShaderResourceView->Release();
					this->_state.pShaderResourceView = nullptr;
				}

				if (this->_state.pIndexBuffer) {
					this->_state.pIndexBuffer->Release();
					this->_state.pIndexBuffer = nullptr;
				}

				if (this->_state.pVertexBuffer) {
					this->_state.pVertexBuffer->Release();
					this->_state.pVertexBuffer = nullptr;
				}

				if (this->_state.pPixelShader) {

					for (UINT i = 0u; i < this->_state.psInstancesCount; i++) {
						this->_state.psInstances[i]->Release();
						this->_state.psInstances[i] = nullptr;
					}

					this->_state.psInstancesCount = 0u;
					this->_state.pPixelShader->Release();
					this->_state.pPixelShader = nullptr;
				}

				if (this->_state.pBlendState) {
					this->_state.pBlendState->Release();
					this->_state.pBlendState = nullptr;
				}

				if (this->_state.pSamplerState) {
					this->_state.pSamplerState->Release();
					this->_state.pSamplerState = nullptr;
				}

				if (this->_state.pConstantBuffer) {
					this->_state.pConstantBuffer->Release();
					this->_state.pConstantBuffer = nullptr;
				}

				if (this->_state.pVertexShader) {

					for (UINT i = 0u; i < this->_state.vsInstancesCount; i++) {
						this->_state.vsInstances[i]->Release();
						this->_state.vsInstances[i] = nullptr;
					}

					this->_state.vsInstancesCount = 0u;
					this->_state.pVertexShader->Release();
					this->_state.pVertexShader = nullptr;
				}

				if (this->_state.pInputLayout) {
					this->_state.pInputLayout->Release();
					this->_state.pInputLayout = nullptr;
				}

				if (this->_state.pRenderTargetView) {
					this->_state.pRenderTargetView->Release();
					this->_state.pRenderTargetView = nullptr;
				}

				if (this->_state.pDepthStencilView) {
					this->_state.pDepthStencilView->Release();
					this->_state.pDepthStencilView = nullptr;
				}

				return;
			}

		}

	}

}