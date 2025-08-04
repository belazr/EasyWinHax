#include "dx10Backend.h"
#include "dx10Shaders.h"
#include "..\..\..\proc.h"

namespace hax {
	
	namespace draw {

		namespace dx10 {

			bool getD3D10SwapChainVTable(void** pSwapChainVTable, size_t size) {
				DXGI_SWAP_CHAIN_DESC swapChainDesc{};
				swapChainDesc.BufferCount = 1u;
				swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				swapChainDesc.OutputWindow = GetDesktopWindow();
				swapChainDesc.Windowed = TRUE;
				swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
				swapChainDesc.SampleDesc.Count = 1u;

				ID3D10Device* pDevice = nullptr;
				IDXGISwapChain* pSwapChain = nullptr;

				const HRESULT hResult = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0u, D3D10_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice);

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
				_pSwapChain{}, _pDevice{}, _pInputLayout{}, _pVertexShader{}, _pPixelShaderTexture{}, _pPixelShaderPassthrough{},
				_pConstantBuffer{}, _pSamplerState{}, _pBlendState{}, _viewport{}, _textureBufferBackend{}, _solidBufferBackend{} {}


			Backend::~Backend() {

				if (this->_pBlendState) {
					this->_pBlendState->Release();
				}

				if (this->_pSamplerState) {
					this->_pSamplerState->Release();
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

				if (!this->_pInputLayout) {
					
					if (!this->createInputLayout()) return false;
				
				}
				
				if (!this->createShaders()) return false;

				constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;

				this->_textureBufferBackend.initialize(this->_pDevice, this->_pPixelShaderTexture);

				if (!this->_textureBufferBackend.capacity()) {

					if (!this->_textureBufferBackend.create(INITIAL_BUFFER_SIZE)) return false;

				}

				this->_solidBufferBackend.initialize(this->_pDevice, this->_pPixelShaderPassthrough);

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
				D3D10_TEXTURE2D_DESC textureDesc{};
				textureDesc.Width = width;
				textureDesc.Height = height;
				textureDesc.MipLevels = 1u;
				textureDesc.ArraySize = 1u;
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				textureDesc.SampleDesc.Count = 1u;
				textureDesc.BindFlags = D3D10_BIND_SHADER_RESOURCE;

				D3D10_SUBRESOURCE_DATA subResData = {};
				subResData.pSysMem = data;
				subResData.SysMemPitch = width * sizeof(Color);

				ID3D10Texture2D* pTexture = nullptr;

				if (FAILED(this->_pDevice->CreateTexture2D(&textureDesc, &subResData, &pTexture))) return 0ull;

				D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc{};
				viewDesc.Format = textureDesc.Format;
				viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;

				ID3D10ShaderResourceView* pTextureView = nullptr;

				if (FAILED(this->_pDevice->CreateShaderResourceView(pTexture, &viewDesc, &pTextureView))) {
					pTexture->Release();

					return 0ull;
				}

				this->_textures.append(TextureData{ pTexture, pTextureView });

				return static_cast<TextureId>(reinterpret_cast<uintptr_t>(pTextureView));
			}


			bool Backend::beginFrame() {
				D3D10_VIEWPORT curViewport{};

				if (!this->getCurrentViewport(&curViewport)) return false;

				if (curViewport.Width != this->_viewport.Width || curViewport.Height != this->_viewport.Height) {

					if (!this->updateConstantBuffer(curViewport)) return false;

					this->_viewport = curViewport;
				}

				this->_pDevice->VSSetConstantBuffers(0u, 1u, &this->_pConstantBuffer);
				this->_pDevice->VSSetShader(this->_pVertexShader);
				this->_pDevice->IASetInputLayout(this->_pInputLayout);
				this->_pDevice->PSSetSamplers(0u, 1u, &this->_pSamplerState);
				
				constexpr float BLEND_FACTOR[] { 0.f, 0.f, 0.f, 0.f };
				this->_pDevice->OMSetBlendState(this->_pBlendState, BLEND_FACTOR, 0xffffffffu);

				return true;
			}


			void Backend::endFrame() {

				return;
			}


			IBufferBackend* Backend::getTextureBufferBackend() {

				return &this->_textureBufferBackend;
			}


			IBufferBackend* Backend::getSolidBufferBackend() {

				return &this->_solidBufferBackend;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = static_cast<float>(this->_viewport.Width);
				*frameHeight = static_cast<float>(this->_viewport.Height);

				return;
			}


			bool Backend::createInputLayout() {
				constexpr D3D10_INPUT_ELEMENT_DESC INPUT_ELEMENT_DESC[]{
					{"POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D10_INPUT_PER_VERTEX_DATA, 0u},
					{"COLOR", 0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0u},
					{"TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0u},
				};

				return SUCCEEDED(this->_pDevice->CreateInputLayout(INPUT_ELEMENT_DESC, _countof(INPUT_ELEMENT_DESC), VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pInputLayout));
			}


			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pVertexShader))) return false;

				}

				if (!this->_pPixelShaderTexture) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_TEXTURE, sizeof(PIXEL_SHADER_TEXTURE), &this->_pPixelShaderTexture))) return false;

				}

				if (!this->_pPixelShaderPassthrough) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH), &this->_pPixelShaderPassthrough))) return false;

				}

				return true;
			}


			bool Backend::createConstantBuffer() {
				D3D10_BUFFER_DESC bufferDesc{};
				bufferDesc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
				bufferDesc.ByteWidth = 16u * sizeof(float);
				bufferDesc.Usage = D3D10_USAGE_DYNAMIC;
				bufferDesc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

				return SUCCEEDED(this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &this->_pConstantBuffer));
			}


			bool Backend::createSamplerState() {
				D3D10_SAMPLER_DESC samplerDesc{};
				samplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
				samplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;

				return SUCCEEDED(this->_pDevice->CreateSamplerState(&samplerDesc, &this->_pSamplerState));
			}


			bool Backend::createBlendState() {
				D3D10_BLEND_DESC blendDesc{};
				blendDesc.AlphaToCoverageEnable = FALSE;
				blendDesc.BlendEnable[0] = TRUE;
				blendDesc.SrcBlend = D3D10_BLEND_SRC_ALPHA;
				blendDesc.DestBlend = D3D10_BLEND_INV_SRC_ALPHA;
				blendDesc.BlendOp = D3D10_BLEND_OP_ADD;
				blendDesc.SrcBlendAlpha = D3D10_BLEND_ONE;
				blendDesc.DestBlendAlpha = D3D10_BLEND_INV_SRC_ALPHA;
				blendDesc.BlendOpAlpha = D3D10_BLEND_OP_ADD;
				blendDesc.RenderTargetWriteMask[0] = D3D10_COLOR_WRITE_ENABLE_ALL;

				return SUCCEEDED(this->_pDevice->CreateBlendState(&blendDesc, &this->_pBlendState));
			}


			bool Backend::getCurrentViewport(D3D10_VIEWPORT* pViewport) const {
				D3D10_VIEWPORT viewports[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
				UINT viewportCount = _countof(viewports);

				this->_pDevice->RSGetViewports(&viewportCount, viewports);

				*pViewport = viewports[0];

				if (!viewportCount || !pViewport->Width) {
					HWND hMainWnd = proc::in::getMainWindowHandle();

					if (!hMainWnd) return false;

					RECT windowRect{};

					if (!GetClientRect(hMainWnd, &windowRect)) return false;

					pViewport->Width = static_cast<UINT>(windowRect.right);
					pViewport->Height = static_cast<UINT>(windowRect.bottom);
					pViewport->TopLeftX = static_cast<UINT>(windowRect.left);
					pViewport->TopLeftY = static_cast<UINT>(windowRect.top);
					pViewport->MinDepth = 0.f;
					pViewport->MaxDepth = 1.f;
					this->_pDevice->RSSetViewports(1u, pViewport);
				}

				return true;
			}


			bool Backend::updateConstantBuffer(D3D10_VIEWPORT viewport) const {
				const float viewLeft = static_cast<float>(viewport.TopLeftX);
				const float viewRight = static_cast<float>(viewport.TopLeftX + viewport.Width);
				const float viewTop = static_cast<float>(viewport.TopLeftY);
				const float viewBottom = static_cast<float>(viewport.TopLeftY + viewport.Height);

				const float ortho[][4]{
					{ 2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f  },
					{ 0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f },
					{ 0.f, 0.f, .5f, 0.f },
					{ (viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f }
				};

				void* pData = nullptr;

				if (FAILED(this->_pConstantBuffer->Map(D3D10_MAP_WRITE_DISCARD, 0u, &pData))) return false;

				memcpy(pData, ortho, sizeof(ortho));

				this->_pConstantBuffer->Unmap();

				return true;
			}

		}

	}

}