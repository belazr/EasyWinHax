#include "dx9Backend.h"
#include "dx9Shaders.h"

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


			Backend::Backend() : _pDevice{}, _pVertexDeclaration{}, _pVertexShader{}, _pPixelShader{}, _viewport{}, _state{} {}


			Backend::~Backend() {
				this->releaseState();

				this->_bufferBackend.destroy();

				if (this->_pPixelShader) {
					this->_pPixelShader->Release();
				}

				if (this->_pVertexShader) {
					this->_pVertexShader->Release();
				}

				if (this->_pVertexDeclaration) {
					this->_pVertexDeclaration->Release();
				}

				for (size_t i = 0u; i < this->_textures.size(); i++) {
					this->_textures[i]->Release();
				}

			}


			void Backend::setHookParameters(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg2);

				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pArg1);

				return;
			}

			
			bool Backend::initialize() {

				if (!this->_pVertexDeclaration) {
					
					if (!this->createVertexDeclaration()) return false;
				
				}

				if (!this->createShaders()) return false;

				constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;

				this->_bufferBackend.initialize(this->_pDevice);

				if (!this->_bufferBackend.capacity()) {

					if (!this->_bufferBackend.create(INITIAL_BUFFER_SIZE)) return false;

				}

				return true;
			}


			TextureId Backend::loadTexture(const Color* data, uint32_t width, uint32_t height) {
				IDirect3DTexture9* pTexture = nullptr;

				if (FAILED(this->_pDevice->CreateTexture(width, height, 1u, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pTexture, nullptr))) return 0ull;

				D3DLOCKED_RECT rect{};

				if (FAILED(pTexture->LockRect(0u, &rect, nullptr, 0u)) || !rect.pBits) {
					pTexture->Release();

					return 0ull;
				}

				uint8_t* dst = reinterpret_cast<uint8_t*>(rect.pBits);

				for (uint32_t i = 0; i < height; i++) {
					memcpy(dst, data + i * width, width * sizeof(Color));
					dst += rect.Pitch;
				}

				if (FAILED(pTexture->UnlockRect(0u))) {
					pTexture->Release();

					return 0ull;
				}

				this->_textures.append(pTexture);

				return static_cast<TextureId>(reinterpret_cast<uintptr_t>(pTexture));
			}


			bool Backend::beginFrame() {
				
				if (!this->saveState()) return false;

				if (FAILED(this->_pDevice->GetViewport(&this->_viewport))) return false;

				if (FAILED(this->_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE))) {
					this->restoreState();

					return false;
				}
				
				if (FAILED(this->_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetSamplerState(0u, D3DSAMP_MINFILTER, D3DTEXF_LINEAR))) {
					this->restoreState();

					return false;
				}

				if (FAILED(this->_pDevice->SetSamplerState(0u, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR))) {
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

				if (!this->setVertexShaderConstants()) {
					this->restoreState();

					return false;
				}

				return true;
			}


			void Backend::endFrame() {
				this->restoreState();

				return;
			}


			IBufferBackend* Backend::getBufferBackend() {

				return &this->_bufferBackend;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = static_cast<float>(this->_viewport.Width);
				*frameHeight = static_cast<float>(this->_viewport.Height);

				return;
			}


			bool Backend::createVertexDeclaration() {
				constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[]{
					{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0u },
					{ 0u, 8u, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0u },
					{ 0u, 12u, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0u },
					D3DDECL_END()
				};
				
				return SUCCEEDED(this->_pDevice->CreateVertexDeclaration(VERTEX_ELEMENTS, &this->_pVertexDeclaration));
			}


			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(VERTEX_SHADER), &this->_pVertexShader))) return false;

				}

				if (!this->_pPixelShader) {

					if (FAILED(this->_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(PIXEL_SHADER), &this->_pPixelShader))) return false;

				}

				return true;
			}


			bool Backend::saveState() {
				
				if (FAILED(this->_pDevice->CreateStateBlock(D3DSBT_ALL, &this->_state.pStateBlock))) return false;

				if (FAILED(this->_pDevice->GetVertexDeclaration(&this->_state.pVertexDeclaration))) return false;

				if (FAILED(this->_pDevice->GetVertexShader(&this->_state.pVertexShader))) return false;
				
				if (FAILED(this->_pDevice->GetPixelShader(&this->_state.pPixelShader))) return false;

				if (FAILED(this->_pDevice->GetStreamSource(0u, &this->_state.pVertexBuffer, &this->_state.offset, &this->_state.stride))) return false;

				if (FAILED(this->_pDevice->GetIndices(&this->_state.pIndexBuffer))) return false;

				if (FAILED(this->_pDevice->GetTexture(0u, &this->_state.pBaseTexture))) return false;

				return true;
			}


			void Backend::restoreState() {
				this->_pDevice->SetTexture(0u, this->_state.pBaseTexture);
				this->_pDevice->SetIndices(this->_state.pIndexBuffer);
				this->_pDevice->SetStreamSource(0u, this->_state.pVertexBuffer, this->_state.offset, this->_state.stride);
				this->_pDevice->SetPixelShader(this->_state.pPixelShader);
				this->_pDevice->SetVertexShader(this->_state.pVertexShader);
				this->_pDevice->SetVertexDeclaration(this->_state.pVertexDeclaration);
				this->_state.pStateBlock->Apply();

				this->releaseState();

				return;
			}


			void Backend::releaseState() {

				if (this->_state.pBaseTexture) {
					this->_state.pBaseTexture->Release();
					this->_state.pBaseTexture = nullptr;
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
					this->_state.pPixelShader->Release();
					this->_state.pPixelShader = nullptr;
				}

				if (this->_state.pVertexShader) {
					this->_state.pVertexShader->Release();
					this->_state.pVertexShader = nullptr;
				}

				if (this->_state.pVertexDeclaration) {
					this->_state.pVertexDeclaration->Release();
					this->_state.pVertexDeclaration = nullptr;
				}

				if (this->_state.pStateBlock) {
					this->_state.pStateBlock->Release();
					this->_state.pStateBlock = nullptr;
				}

				return;
			}


			bool Backend::setVertexShaderConstants() {
				// adding .5f for mapping texels exactly to pixels
				// https://learn.microsoft.com/en-us/windows/win32/direct3d9/directly-mapping-texels-to-pixels
				const float left = static_cast<float>(this->_viewport.X) + .5f;
				const float right = static_cast<float>(this->_viewport.X + this->_viewport.Width);
				const float top = static_cast<float>(this->_viewport.Y) + .5f;
				const float bottom = static_cast<float>(this->_viewport.Y + this->_viewport.Height);

				const float ortho[]{
					2.f / (right - left), 0.f, 0.f, 0.f,
					0.f, 2.f / (top - bottom), 0.f, 0.f,
					0.f, 0.f, .5f, 0.f,
					(left + right) / (left - right), (top + bottom) / (bottom - top), .5f, 1.f
				};

				return SUCCEEDED(this->_pDevice->SetVertexShaderConstantF(0u, ortho, 4u));
			}

		}

	}

}