#include "dx11Backend.h"

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
				_pSwapChain{}, _pDevice{}, _pContext{}, _pVertexShader{}, _pVertexLayout{}, _pPixelShaderPassthrough{}, _pPixelShaderTexture{},
				_pConstantBuffer{}, _pSamplerState{}, _pBlendState{}, _pRenderTargetView{}, _viewport{}, _triangleListBuffer{}, _pointListBuffer{} {}


			Backend::~Backend() {

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

				if (this->_pContext) {
					this->_pointListBuffer.destroy();
					this->_triangleListBuffer.destroy();
				}

				if (this->_pPixelShaderTexture) {
					this->_pPixelShaderTexture->Release();
				}

				if (this->_pPixelShaderPassthrough) {
					this->_pPixelShaderPassthrough->Release();
				}

				if (this->_pVertexLayout) {
					this->_pVertexLayout->Release();
				}

				if (this->_pVertexShader) {
					this->_pVertexShader->Release();
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


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
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

				if (!this->createShaders()) return false;

				this->_triangleListBuffer.initialize(this->_pDevice, this->_pContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, this->_pPixelShaderPassthrough, this->_pPixelShaderTexture);
				this->_triangleListBuffer.destroy();

				constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

				if (!this->_triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

				this->_pointListBuffer.initialize(this->_pDevice, this->_pContext, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, this->_pPixelShaderPassthrough, this->_pPixelShaderTexture);
				this->_pointListBuffer.destroy();

				constexpr uint32_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;

				if (!this->_pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) return false;

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

				this->_pContext->OMSetRenderTargets(1u, &this->_pRenderTargetView, nullptr);
				this->_pContext->VSSetConstantBuffers(0u, 1u, &this->_pConstantBuffer);
				this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0u);
				this->_pContext->IASetInputLayout(this->_pVertexLayout);
				this->_pContext->PSSetSamplers(0u, 1u, &this->_pSamplerState);

				constexpr float BLEND_FACTOR[]{ 0.f, 0.f, 0.f, 0.f };
				this->_pContext->OMSetBlendState(this->_pBlendState, BLEND_FACTOR, 0xffffffffu);

				return true;
			}


			void Backend::endFrame() {
				this->_pRenderTargetView->Release();
				this->_pRenderTargetView = nullptr;

				return;
			}


			AbstractDrawBuffer* Backend::getTriangleListBuffer() {

				return &this->_triangleListBuffer;
			}


			AbstractDrawBuffer* Backend::getPointListBuffer() {

				return &this->_pointListBuffer;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = this->_viewport.Width;
				*frameHeight = this->_viewport.Height;

				return;
			}


			/*
			cbuffer vertexBuffer : register(b0) {
				float4x4 projectionMatrix;
			}

			struct VSI {
				float2 pos : POSITION;
				float4 col : COLOR0;
				float2 uv : TEXCOORD0;
			};

			struct PSI {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv : TEXCOORD0;
			};

			PSI main(VSI input) {
				PSI output;
				output.pos = mul(projectionMatrix, float4(input.pos.xy, 0.f, 1.f));
				output.col = input.col;
				output.uv = input.uv;

				return output;
			}
			*/
			static constexpr BYTE VERTEX_SHADER[]{
				0x44, 0x58, 0x42, 0x43, 0x79, 0x5A, 0xE9, 0xF2, 0x55, 0xA2, 0x3F, 0x5A, 0xD6, 0x54, 0xA9, 0xF5,
				0xD6, 0xF9, 0xE0, 0xC8, 0x01, 0x00, 0x00, 0x00, 0xDC, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0x50, 0x01, 0x00, 0x00, 0xC0, 0x01, 0x00, 0x00, 0x34, 0x02, 0x00, 0x00,
				0x40, 0x03, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x14, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
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
				0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x70, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69,
				0x6F, 0x6E, 0x4D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x00, 0x66, 0x6C, 0x6F, 0x61, 0x74, 0x34, 0x78,
				0x34, 0x00, 0xAB, 0xAB, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0xBD, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
				0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65,
				0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00,
				0x49, 0x53, 0x47, 0x4E, 0x68, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
				0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x50, 0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E,
				0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4F, 0x4F, 0x52, 0x44, 0x00,
				0x4F, 0x53, 0x47, 0x4E, 0x6C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
				0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54,
				0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4F, 0x4F,
				0x52, 0x44, 0x00, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x04, 0x01, 0x00, 0x00, 0x50, 0x00, 0x01, 0x00,
				0x41, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x59, 0x00, 0x00, 0x04, 0x46, 0x8E, 0x20, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x5F, 0x00, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x04,
				0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03,
				0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0x32, 0x20, 0x10, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x08,
				0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x15, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x0A,
				0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x06, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x05, 0x32, 0x20, 0x10, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01,
				0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			/*
			struct PSI {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv : TEXCOORD0;
			};

			float4 main(PSI input) : SV_TARGET{

				return input.col;
			}
			*/
			static constexpr BYTE PIXEL_SHADER_PASSTHROUGH[]{
				0x44, 0x58, 0x42, 0x43, 0x8B, 0x91, 0x2C, 0x03, 0x48, 0x9A, 0xD8, 0xE2, 0xED, 0x82, 0x83, 0xA8,
				0x67, 0xF3, 0xC5, 0xFB, 0x01, 0x00, 0x00, 0x00, 0x28, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x48, 0x01, 0x00, 0x00,
				0x8C, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF,
				0x00, 0x01, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00,
				0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
				0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
				0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65,
				0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00,
				0x49, 0x53, 0x47, 0x4E, 0x6C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
				0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54,
				0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4F, 0x4F,
				0x52, 0x44, 0x00, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54,
				0x41, 0x52, 0x47, 0x45, 0x54, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0x3C, 0x00, 0x00, 0x00,
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

			/*
			Texture2D tex : register(t0);
			SamplerState texSampler : register(s0);

			struct PSI {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv : TEXCOORD0;
			};

			float4 main(PSI input) : SV_Target {
				float4 texColor = tex.Sample(texSampler, input.uv);

				return texColor * input.col;
			}
			*/
			static constexpr BYTE PIXEL_SHADER_TEXTURE[]{
				0x44, 0x58, 0x42, 0x43, 0x35, 0xA0, 0x4A, 0x30, 0x4A, 0xDD, 0xC8, 0x50, 0xCC, 0x6E, 0x39, 0x1E,
				0xC9, 0x1D, 0xDA, 0xD2, 0x01, 0x00, 0x00, 0x00, 0xDC, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x64, 0x01, 0x00, 0x00, 0x98, 0x01, 0x00, 0x00,
				0x40, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x05, 0xFF, 0xFF,
				0x00, 0x01, 0x00, 0x00, 0x8B, 0x00, 0x00, 0x00, 0x52, 0x44, 0x31, 0x31, 0x3C, 0x00, 0x00, 0x00,
				0x18, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
				0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x74, 0x65, 0x78, 0x53, 0x61, 0x6D, 0x70, 0x6C,
				0x65, 0x72, 0x00, 0x74, 0x65, 0x78, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74,
				0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
				0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB,
				0x49, 0x53, 0x47, 0x4E, 0x6C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x00, 0x00,
				0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50, 0x4F, 0x53, 0x49, 0x54,
				0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4F, 0x4F,
				0x52, 0x44, 0x00, 0xAB, 0x4F, 0x53, 0x47, 0x4E, 0x2C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x54,
				0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x45, 0x58, 0xA0, 0x00, 0x00, 0x00,
				0x50, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x6A, 0x08, 0x00, 0x01, 0x5A, 0x00, 0x00, 0x03,
				0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02,
				0x01, 0x00, 0x00, 0x00, 0x45, 0x00, 0x00, 0x8B, 0xC2, 0x00, 0x00, 0x80, 0x43, 0x55, 0x15, 0x00,
				0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x46, 0x7E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x38, 0x00, 0x00, 0x07, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x0E, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01,
				0x53, 0x54, 0x41, 0x54, 0x94, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(VERTEX_SHADER, sizeof(VERTEX_SHADER), nullptr, &this->_pVertexShader))) return false;

				}

				constexpr D3D11_INPUT_ELEMENT_DESC INPUT_ELEMENT_DESC[]{
					{"POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u},
					{"COLOR", 0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u},
					{"TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u}
				};

				if (!this->_pVertexLayout) {

					if (FAILED(this->_pDevice->CreateInputLayout(INPUT_ELEMENT_DESC, _countof(INPUT_ELEMENT_DESC), VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pVertexLayout))) return false;

				}

				if (!this->_pPixelShaderPassthrough) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH), nullptr, &this->_pPixelShaderPassthrough))) return false;

				}

				if (!this->_pPixelShaderTexture) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_TEXTURE, sizeof(PIXEL_SHADER_TEXTURE), nullptr, &this->_pPixelShaderTexture))) return false;

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
				samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
				samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
				samplerDesc.MipLODBias = 0.f;
				samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
				samplerDesc.MinLOD = 0.f;
				samplerDesc.MaxLOD = 0.f;

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


			static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

			bool Backend::getCurrentViewport(D3D11_VIEWPORT* pViewport) const {
				D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
				UINT viewportCount = sizeof(viewports) / sizeof(D3D11_VIEWPORT);

				this->_pContext->RSGetViewports(&viewportCount, viewports);

				*pViewport = viewports[0];

				if (!viewportCount || !pViewport->Width) {
					HWND hMainWnd = nullptr;

					EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

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