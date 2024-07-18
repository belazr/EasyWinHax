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


			Backend::Backend() : _pDevice{}, _pOriginalVertexDeclaration{}, _pVertexDeclaration{}, _viewport{}, _triangleListBuffer{}, _pointListBuffer{} {}


			Backend::~Backend() {

				this->_pointListBuffer.destroy();
				this->_triangleListBuffer.destroy();

				if (this->_pVertexDeclaration) {
					this->_pVertexDeclaration->Release();
				}

			}


			void Backend::setHookArguments(void* pArg1, const void* pArg2, void* pArg3) {
				UNREFERENCED_PARAMETER(pArg2);
				UNREFERENCED_PARAMETER(pArg3);

				this->_pDevice = reinterpret_cast<IDirect3DDevice9*>(pArg1);

				return;
			}

			
			bool Backend::initialize() {

				if (!this->_pVertexDeclaration) {
					constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[]{
						{ 0u, 0u,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0u },
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
				
				if(FAILED(this->_pDevice->GetViewport(&this->_viewport))) return false;

				if (!this->_pOriginalVertexDeclaration) {

					if (FAILED(this->_pDevice->GetVertexDeclaration(&this->_pOriginalVertexDeclaration))) return false;

				}

				if (FAILED(this->_pDevice->SetVertexDeclaration(this->_pVertexDeclaration))) {
					this->_pOriginalVertexDeclaration->Release();
					this->_pOriginalVertexDeclaration = nullptr;

					return false;
				}

				return true;
			}


			void Backend::endFrame() {
				this->_pDevice->SetVertexDeclaration(this->_pOriginalVertexDeclaration);
				this->_pOriginalVertexDeclaration->Release();
				this->_pOriginalVertexDeclaration = nullptr;

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

		}

	}

}