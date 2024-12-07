#include "dx9Backend.h"

namespace hax {

	namespace draw {

		namespace dx9 {

			bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
				IDirect3D9* const pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

				if (!pDirect3D9) return false;

				D3DPRESENT_PARAMETERS d3dpp{};
				d3dpp.hDeviceWindow = GetDesktopWindow();
				d3dpp.Windowed = TRUE;
				d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

				IDirect3DDevice9* pDirect3D9Device = nullptr;

				HRESULT hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

				if (hResult != D3D_OK) {
					pDirect3D9->Release();

					return false;
				}

				if (memcpy_s(pDeviceVTable, size, *reinterpret_cast<void**>(pDirect3D9Device), size)) {
					pDirect3D9Device->Release();
					pDirect3D9->Release();

					return false;
				}

				pDirect3D9Device->Release();
				pDirect3D9->Release();

				return true;
			}


			Backend::Backend() : _pDevice{}, _pVertexDeclaration{}, _pVertexShader{}, _pPixelShader{}, _viewport{},
				_pStateBlock{}, _pOriginalVertexDeclaration{}, _triangleListBuffer{}, _pointListBuffer{} {}


			Backend::~Backend() {

				if (this->_pOriginalVertexDeclaration) {
					this->_pOriginalVertexDeclaration->Release();
				}
				
				if (this->_pStateBlock) {
					this->_pStateBlock->Release();
				}

				this->_pointListBuffer.destroy();
				this->_triangleListBuffer.destroy();

				if (this->_pPixelShader) {
					this->_pPixelShader->Release();
				}

				if (this->_pVertexShader) {
					this->_pVertexShader->Release();
				}

				if (this->_pVertexDeclaration) {
					this->_pVertexDeclaration->Release();
				}

			}


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg2);

				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pArg1);

				return;
			}

			
			bool Backend::initialize() {

				if (!this->createShaders()) return false;

				if (!this->_pVertexDeclaration) {
					constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[]{
						{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0u },
						{ 0u, 8u, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0u },
						D3DDECL_END()
					};

					if (FAILED(this->_pDevice->CreateVertexDeclaration(VERTEX_ELEMENTS, &this->_pVertexDeclaration))) return false;

				}

				this->_triangleListBuffer.initialize(this->_pDevice, D3DPT_TRIANGLELIST);
				this->_triangleListBuffer.destroy();

				constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

				if (!this->_triangleListBuffer.create(INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return false;

				this->_pointListBuffer.initialize(this->_pDevice, D3DPT_POINTLIST);
				this->_pointListBuffer.destroy();

				constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 1000u;

				if (!this->_pointListBuffer.create(INITIAL_POINT_LIST_BUFFER_SIZE)) return false;

				return true;
			}


			bool Backend::beginFrame() {

				if (FAILED(this->_pDevice->GetViewport(&this->_viewport))) return false;

				if (FAILED(this->_pDevice->CreateStateBlock(D3DSBT_ALL, &this->_pStateBlock))) return false;

				if (FAILED(this->_pDevice->GetVertexDeclaration(&this->_pOriginalVertexDeclaration))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetVertexDeclaration(this->_pVertexDeclaration))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetVertexShader(this->_pVertexShader))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetPixelShader(this->_pPixelShader))) {
					this->restoreState();

					return false;
				}

				if (!this->setVertexShaderConstant())) {
					this->restoreState();

					return false;
				}

				return true;
			}


			void Backend::endFrame() {
				this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
				this->_pOriginalVertexDeclaration->Release();
				this->_pOriginalVertexDeclaration = nullptr;
				this->restoreState();

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
			float4x4 projectionMatrix : register(c0);

			struct VSI {
				float2 pos : POSITION;
				float4 col : COLOR0;
			};

			struct PSI {
				float4 pos : POSITION;
				float4 col : COLOR0;
			};

			PSI main(VSI input) {
				PSI output;
				output.pos = mul(projectionMatrix, float4(input.pos, 0.f, 1.f));
				output.col = input.col;
				return output;
			}
			*/
			static constexpr BYTE VERTEX_SHADER[]{
				0x00, 0x03, 0xFE, 0xFF, 0xFE, 0xFF, 0x22, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
				0x5B, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFE, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00,
				0x00, 0x01, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
				0x04, 0x00, 0x02, 0x00, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x72, 0x6F, 0x6A,
				0x65, 0x63, 0x74, 0x69, 0x6F, 0x6E, 0x4D, 0x61, 0x74, 0x72, 0x69, 0x78, 0x00, 0xAB, 0xAB, 0xAB,
				0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x76, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D, 0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74,
				0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53, 0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72,
				0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65, 0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB,
				0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0x90, 0x1F, 0x00, 0x00, 0x02,
				0x0A, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0x90, 0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x0F, 0xE0, 0x1F, 0x00, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0xE0,
				0x05, 0x00, 0x00, 0x03, 0x00, 0x00, 0x0F, 0x80, 0x01, 0x00, 0xE4, 0xA0, 0x00, 0x00, 0x55, 0x90,
				0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0xE4, 0xA0, 0x00, 0x00, 0x00, 0x90,
				0x00, 0x00, 0xE4, 0x80, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0xE4, 0x80,
				0x03, 0x00, 0xE4, 0xA0, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0F, 0xE0, 0x01, 0x00, 0xE4, 0x90,
				0xFF, 0xFF, 0x00, 0x00
			};

			/*
			struct PSI {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
			};

			float4 main(PSI input) : SV_Target {
				return input.col;
			}
			*/
			static constexpr BYTE PIXEL_SHADER[]{
				0x00, 0x03, 0xFF, 0xFF, 0xFE, 0xFF, 0x14, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
				0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x70, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
				0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
				0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
				0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB, 0x1F, 0x00, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x80,
				0x00, 0x00, 0x0F, 0x90, 0x01, 0x00, 0x00, 0x02, 0x00, 0x08, 0x0F, 0x80, 0x00, 0x00, 0xE4, 0x90,
				0xFF, 0xFF, 0x00, 0x00
			};

			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(VERTEX_SHADER), &this->_pVertexShader))) return false;

				}
				
				if (!this->_pPixelShader) {

					if (FAILED(this->_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(PIXEL_SHADER), &this->_pPixelShader))) return false;

				}

				return true;
			}

			void Backend::restoreState() {

				if (this->_pOriginalVertexDeclaration) {
					this->_pOriginalVertexDeclaration->Release();
					this->_pOriginalVertexDeclaration = nullptr;
				}

				if (this->_pStateBlock) {
					this->_pStateBlock->Apply();
					this->_pStateBlock->Release();
					this->_pStateBlock = nullptr;
				}

				return;
			}

			bool Backend::setVertexShaderConstant() {
				const float viewLeft = static_cast<float>(this->_viewport.X);
				const float viewRight = static_cast<float>(this->_viewport.X + this->_viewport.Width);
				const float viewTop = static_cast<float>(this->_viewport.Y);
				const float viewBottom = static_cast<float>(this->_viewport.Y + this->_viewport.Height);

				const float ortho[]{
					2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f,
					0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f,
					0.f, 0.f, .5f, 0.f,
					(viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f
				};

				return SUCCEEDED(this->_pDevice->SetVertexShaderConstantF(0u, ortho, 4u));
			}

		}

	}

}