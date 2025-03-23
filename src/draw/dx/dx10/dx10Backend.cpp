#include "dx10Backend.h"

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
				_pSwapChain{}, _pDevice{}, _pVertexShader{}, _pVertexLayout{}, _pPixelShaderPassthrough{}, _pPixelShaderTexture{},
				_pConstantBuffer{}, _pSamplerState{}, _pBlendState{}, _viewport{}, _triangleListBuffer{}, _pointListBuffer{} {}


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

				if (this->_pDevice) {
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
				
				if (!this->createShaders()) return false;

				this->_triangleListBuffer.initialize(this->_pDevice, D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST, this->_pPixelShaderPassthrough, this->_pPixelShaderTexture);
				this->_triangleListBuffer.destroy();

				constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

				if (!this->_triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

				this->_pointListBuffer.initialize(this->_pDevice, D3D10_PRIMITIVE_TOPOLOGY_POINTLIST, this->_pPixelShaderPassthrough, this->_pPixelShaderTexture);
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


			void* Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
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

				if (FAILED(this->_pDevice->CreateTexture2D(&textureDesc, &subResData, &pTexture))) return nullptr;

				D3D10_SHADER_RESOURCE_VIEW_DESC viewDesc{};
				viewDesc.Format = textureDesc.Format;
				viewDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipLevels = textureDesc.MipLevels;

				ID3D10ShaderResourceView* pTextureView = nullptr;

				if (FAILED(this->_pDevice->CreateShaderResourceView(pTexture, &viewDesc, &pTextureView))) {
					pTexture->Release();

					return nullptr;
				}

				this->_textures.append(TextureData{ pTexture, pTextureView });

				return pTextureView;
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
				this->_pDevice->IASetInputLayout(this->_pVertexLayout);
				this->_pDevice->PSSetSamplers(0u, 1u, &this->_pSamplerState);
				
				constexpr float BLEND_FACTOR[] { 0.f, 0.f, 0.f, 0.f };
				this->_pDevice->OMSetBlendState(this->_pBlendState, BLEND_FACTOR, 0xffffffffu);

				return true;
			}


			void Backend::endFrame() {

				return;
			}


			AbstractDrawBuffer* Backend::getTriangleListBuffer() {

				return &this->_triangleListBuffer;
			}


			AbstractDrawBuffer* Backend::getPointListBuffer() {

				return &this->_pointListBuffer;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = static_cast<float>(this->_viewport.Width);
				*frameHeight = static_cast<float>(this->_viewport.Height);

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
				0x44, 0x58, 0x42, 0x43, 0xD5, 0xA2, 0xB0, 0x26, 0x1D, 0xC4, 0xBF, 0xDC, 0x13, 0x20, 0xD5, 0x95,
				0x18, 0x0A, 0xA2, 0xC6, 0x01, 0x00, 0x00, 0x00, 0x6C, 0x03, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x74, 0x01, 0x00, 0x00, 0xE8, 0x01, 0x00, 0x00,
				0xF0, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0xC8, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x4C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x04, 0xFE, 0xFF,
				0x00, 0x01, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x76, 0x65, 0x72, 0x74, 0x65, 0x78, 0x42, 0x75,
				0x66, 0x66, 0x65, 0x72, 0x00, 0xAB, 0xAB, 0xAB, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x64, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x72, 0x6F, 0x6A, 0x65, 0x63, 0x74, 0x69,
				0x6F, 0x6E, 0x4D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x00, 0xAB, 0xAB, 0xAB, 0x03, 0x00, 0x03, 0x00,
				0x04, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72,
				0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53,
				0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31,
				0x30, 0x2E, 0x31, 0x00, 0x49, 0x53, 0x47, 0x4E, 0x68, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x0F, 0x0F, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x50, 0x4F, 0x53, 0x49,
				0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45, 0x58, 0x43, 0x4F,
				0x4F, 0x52, 0x44, 0x00, 0x4F, 0x53, 0x47, 0x4E, 0x6C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x08, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x0F, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x00, 0x00, 0x53, 0x56, 0x5F, 0x50,
				0x4F, 0x53, 0x49, 0x54, 0x49, 0x4F, 0x4E, 0x00, 0x43, 0x4F, 0x4C, 0x4F, 0x52, 0x00, 0x54, 0x45,
				0x58, 0x43, 0x4F, 0x4F, 0x52, 0x44, 0x00, 0xAB, 0x53, 0x48, 0x44, 0x52, 0x00, 0x01, 0x00, 0x00,
				0x40, 0x00, 0x01, 0x00, 0x40, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x04, 0x46, 0x8E, 0x20, 0x00,
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
				0x53, 0x54, 0x41, 0x54, 0x74, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
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
				0x44, 0x58, 0x42, 0x43, 0xE6, 0x06, 0x58, 0x8D, 0x8A, 0xEE, 0x41, 0x53, 0x6B, 0x0C, 0x18, 0xA4,
				0x1D, 0x8A, 0x60, 0xBE, 0x01, 0x00, 0x00, 0x00, 0xE4, 0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xF4, 0x00, 0x00, 0x00, 0x28, 0x01, 0x00, 0x00,
				0x68, 0x01, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF,
				0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66,
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
				0x41, 0x52, 0x47, 0x45, 0x54, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x44, 0x52, 0x38, 0x00, 0x00, 0x00,
				0x40, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x36, 0x00, 0x00, 0x05, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x74, 0x00, 0x00, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00
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
				0x44, 0x58, 0x42, 0x43, 0x87, 0xB4, 0xD6, 0x17, 0x4D, 0x80, 0xB3, 0x69, 0xE6, 0xAD, 0x86, 0x4E,
				0xE3, 0x08, 0x04, 0xF9, 0x01, 0x00, 0x00, 0x00, 0x90, 0x02, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
				0x34, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x44, 0x01, 0x00, 0x00, 0x78, 0x01, 0x00, 0x00,
				0x14, 0x02, 0x00, 0x00, 0x52, 0x44, 0x45, 0x46, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x04, 0xFF, 0xFF,
				0x00, 0x01, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x5C, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x67, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
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
				0x61, 0x72, 0x67, 0x65, 0x74, 0x00, 0xAB, 0xAB, 0x53, 0x48, 0x44, 0x52, 0x94, 0x00, 0x00, 0x00,
				0x40, 0x00, 0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x5A, 0x00, 0x00, 0x03, 0x00, 0x60, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x58, 0x18, 0x00, 0x04, 0x00, 0x70, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x55, 0x55, 0x00, 0x00, 0x62, 0x10, 0x00, 0x03, 0xF2, 0x10, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x62, 0x10, 0x00, 0x03, 0x32, 0x10, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x03,
				0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x02, 0x01, 0x00, 0x00, 0x00,
				0x45, 0x00, 0x00, 0x09, 0xF2, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x10, 0x10, 0x00,
				0x02, 0x00, 0x00, 0x00, 0x46, 0x7E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x10, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x07, 0xF2, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x46, 0x0E, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x1E, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x3E, 0x00, 0x00, 0x01, 0x53, 0x54, 0x41, 0x54, 0x74, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
				0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			};

			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pVertexShader))) return false;

				}

				constexpr D3D10_INPUT_ELEMENT_DESC INPUT_ELEMENT_DESC[]{
					{"POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D10_INPUT_PER_VERTEX_DATA, 0u},
					{"COLOR", 0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0u},
					{"TEXCOORD", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, D3D10_APPEND_ALIGNED_ELEMENT, D3D10_INPUT_PER_VERTEX_DATA, 0u},
				};

				if (!this->_pVertexLayout) {

					if (FAILED(this->_pDevice->CreateInputLayout(INPUT_ELEMENT_DESC, _countof(INPUT_ELEMENT_DESC), VERTEX_SHADER, sizeof(VERTEX_SHADER), &this->_pVertexLayout))) return false;

				}

				if (!this->_pPixelShaderPassthrough) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_PASSTHROUGH, sizeof(PIXEL_SHADER_PASSTHROUGH), &this->_pPixelShaderPassthrough))) return false;

				}

				if (!this->_pPixelShaderTexture) {

					if (FAILED(this->_pDevice->CreatePixelShader(PIXEL_SHADER_TEXTURE, sizeof(PIXEL_SHADER_TEXTURE), &this->_pPixelShaderTexture))) return false;

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
				samplerDesc.Filter = D3D10_FILTER_MIN_MAG_MIP_POINT;
				samplerDesc.AddressU = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressV = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.AddressW = D3D10_TEXTURE_ADDRESS_WRAP;
				samplerDesc.MipLODBias = 0.f;
				samplerDesc.ComparisonFunc = D3D10_COMPARISON_ALWAYS;
				samplerDesc.MinLOD = 0.f;
				samplerDesc.MaxLOD = 0.f;

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


			static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

			bool Backend::getCurrentViewport(D3D10_VIEWPORT* pViewport) const {
				D3D10_VIEWPORT viewports[D3D10_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
				UINT viewportCount = _countof(viewports);

				this->_pDevice->RSGetViewports(&viewportCount, viewports);

				*pViewport = viewports[0];

				if (!viewportCount || !pViewport->Width) {
					HWND hMainWnd = nullptr;

					EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

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