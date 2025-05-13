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


			Backend::Backend() : _pDevice{}, _pVertexDeclaration{}, _pVertexShader{}, _pPixelShaderPassthrough{}, _pPixelShaderTexture{}, _viewport{},
				_pStateBlock{}, _pOriginalVertexDeclaration{}, _triangleListBuffer{}, _textureTriangleListBuffer{} {}


			Backend::~Backend() {

				if (this->_pOriginalVertexDeclaration) {
					this->_pOriginalVertexDeclaration->Release();
				}
				
				if (this->_pStateBlock) {
					this->_pStateBlock->Release();
				}

				this->_textureTriangleListBuffer.destroy();
				this->_triangleListBuffer.destroy();

				if (this->_pPixelShaderTexture) {
					this->_pPixelShaderTexture->Release();
				}

				if (this->_pPixelShaderPassthrough) {
					this->_pPixelShaderPassthrough->Release();
				}

				if (this->_pVertexShader) {
					this->_pVertexShader->Release();
				}

				if (this->_pVertexDeclaration) {
					this->_pVertexDeclaration->Release();
				}

				for (size_t i = 0; i < this->_textures.size(); i++) {
					this->_textures[i]->Release();
				}

			}


			void Backend::setHookArguments(void* pArg1, void* pArg2) {
				UNREFERENCED_PARAMETER(pArg2);

				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pArg1);

				return;
			}

			
			bool Backend::initialize() {

				if (!this->_pVertexDeclaration) {
					constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[]{
						{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0u },
						{ 0u, 8u, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0u },
						{ 0u, 12u, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0u },
						D3DDECL_END()
					};

					if (FAILED(this->_pDevice->CreateVertexDeclaration(VERTEX_ELEMENTS, &this->_pVertexDeclaration))) return false;

				}

				if (!this->createShaders()) return false;

				constexpr uint32_t INITIAL_BUFFER_SIZE = 100u;
				this->_triangleListBuffer.initialize(this->_pDevice, this->_pPixelShaderPassthrough);

				if (!this->_triangleListBuffer.capacity()) {

					if (!this->_triangleListBuffer.create(INITIAL_BUFFER_SIZE)) return false;

				}

				this->_textureTriangleListBuffer.initialize(this->_pDevice, this->_pPixelShaderTexture);

				if (!this->_textureTriangleListBuffer.capacity()) {

					if (!this->_textureTriangleListBuffer.create(INITIAL_BUFFER_SIZE)) return false;

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

				if (!this->setVertexShaderConstant()) {
					this->restoreState();

					return false;
				}

				return true;
			}


			void Backend::endFrame() {
				this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
				this->restoreState();

				return;
			}


			IBufferBackend* Backend::getTriangleListBufferBackend()  {

				return &this->_triangleListBuffer;
			}


			IBufferBackend* Backend::getTextureTriangleListBufferBackend()  {

				return &this->_textureTriangleListBuffer;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) const {
				*frameWidth = static_cast<float>(this->_viewport.Width);
				*frameHeight = static_cast<float>(this->_viewport.Height);

				return;
			}


			bool Backend::createShaders() {

				if (!this->_pVertexShader) {

					if (FAILED(this->_pDevice->CreateVertexShader(reinterpret_cast<const DWORD*>(VERTEX_SHADER), &this->_pVertexShader))) return false;

				}
				
				if (!this->_pPixelShaderPassthrough) {

					if (FAILED(this->_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(PIXEL_SHADER_PASSTHROUGH), &this->_pPixelShaderPassthrough))) return false;

				}

				if (!this->_pPixelShaderTexture) {

					if (FAILED(this->_pDevice->CreatePixelShader(reinterpret_cast<const DWORD*>(PIXEL_SHADER_TEXTURE), &this->_pPixelShaderTexture))) return false;

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