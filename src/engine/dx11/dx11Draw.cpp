#include "dx11Draw.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

namespace hax {

	namespace dx11 {

		bool getD3D11SwapChainVTable(void** pSwapChainVTable, size_t size) {
			DXGI_SWAP_CHAIN_DESC swapChainDesc{};
			swapChainDesc.BufferCount = 1;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapChainDesc.OutputWindow = GetDesktopWindow();
			swapChainDesc.Windowed = TRUE;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
			swapChainDesc.SampleDesc.Count = 1;

			ID3D11Device* _pDevice = nullptr;
			IDXGISwapChain* _pSwapChain = nullptr;

			HRESULT hResult = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &swapChainDesc, &_pSwapChain, &_pDevice, nullptr, nullptr);

			if (hResult != S_OK || !_pDevice || !_pSwapChain) return false;

			if (memcpy_s(pSwapChainVTable, size, *reinterpret_cast<void**>(_pSwapChain), size)) {
				_pDevice->Release();
				_pSwapChain->Release();

				return false;
			}

			_pDevice->Release();
			_pSwapChain->Release();

			return true;
		}


		Draw::Draw() :
			_pSwapChain(nullptr), _pDevice(nullptr), _pContext(nullptr), _pRenderTargetView(nullptr), _pVertexShader(nullptr),
			_pVertexLayout(nullptr), _pPixelShader(nullptr), _pConstantBuffer(nullptr), _viewport{}
		{}


		void Draw::beginDraw(const Engine* pEngine) {

			if (!this->_pSwapChain) {
				this->_pSwapChain = reinterpret_cast<IDXGISwapChain*>(const_cast<hax::Engine*>(pEngine));
				const HRESULT hResult = this->_pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&this->_pDevice));

				if (hResult != S_OK || !this->_pDevice) return;

				this->_pDevice->GetImmediateContext(&this->_pContext);

				if (!this->_pContext) return;

				this->_pContext->OMGetRenderTargets(1, &this->_pRenderTargetView, nullptr);

				if (!this->_pRenderTargetView) {
					this->_pRenderTargetView = createRenderTargetView();
					this->_pContext->OMSetRenderTargets(1, &this->_pRenderTargetView, nullptr);
				}

				if (!this->compileShaders()) return;
				if (!this->getViewport()) return;
				if (!this->setupOrtho()) return;

			}

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const {
			UNREFERENCED_PARAMETER(pEngine);

			return;
		}


		typedef struct Vertex{
			DirectX::XMFLOAT3 pos;
			D3DCOLORVALUE color;
		}Vertex;

		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {
			Vertex pVerts[2]{
				{ DirectX::XMFLOAT3(0, 0, 1.0f), { 0.0f, 1.0f, 0.0f, 1.0f } },
				{ DirectX::XMFLOAT3(400, 400, 1.0f), { 0.0f, 1.0f, 0.0f, 1.0f } },
			};

			D3D11_BUFFER_DESC bufferDescLine{ 0 };
			bufferDescLine.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDescLine.ByteWidth = sizeof(Vertex) * 2;
			bufferDescLine.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pSysMem = &pVerts;

			ID3D11Buffer* pVertexBuffer = nullptr;

			this->_pDevice->CreateBuffer(&bufferDescLine, &subresourceData, &pVertexBuffer);
			this->_pContext->VSSetConstantBuffers(0, 1, &this->_pConstantBuffer);

			UINT stride = sizeof(Vertex);
			UINT offset = 0;

			this->_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0);
			this->_pContext->RSSetViewports(1, &this->_viewport);
			this->_pContext->Draw(2, 0);

			pVertexBuffer->Release();

			return;
		}


		void Draw::drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const {

			return;
		}


		ID3D11RenderTargetView* Draw::createRenderTargetView() {
			ID3D11Texture2D* pBackBuffer = nullptr;
			HRESULT hResult = this->_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
				
			if (hResult != S_OK || !pBackBuffer) return nullptr;

			ID3D11RenderTargetView* pRenderTargetView = nullptr;

			hResult = this->_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
			pBackBuffer->Release();
				
			if (hResult != S_OK || !pRenderTargetView) return nullptr;

			return pRenderTargetView;
		}


		constexpr const char* shader = R"(
			cbuffer ConstantBuffer : register(b0)
			{
				matrix projection;
			}

			struct PSI
			{
				float4 pos : SV_POSITION;
				float4 color : COLOR;
			};

			PSI VS( float4 pos : POSITION, float4 color : COLOR )
			{
				PSI psi;
				psi.pos = mul( projection, pos  );
				psi.color = color;
				return psi;
			}

			float4 PS(PSI psi) : SV_TARGET
			{
				return psi.color;
			}
		)";

		bool Draw::compileShaders() {
			ID3D10Blob* pCompiledVertexShader = nullptr;
			HRESULT hResult = D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, "VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pCompiledVertexShader, nullptr);
			
			if (hResult != S_OK || !pCompiledVertexShader) return false;

			hResult = this->_pDevice->CreateVertexShader(pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), nullptr, &this->_pVertexShader);
			
			if (hResult != S_OK || !this->_pVertexShader) return false;

			D3D11_INPUT_ELEMENT_DESC inputElementDesc[2] {
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
			};

			hResult = this->_pDevice->CreateInputLayout(inputElementDesc, sizeof(inputElementDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), &this->_pVertexLayout);

			if (hResult != S_OK) return false;

			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			
			pCompiledVertexShader->Release();

			ID3D10Blob* pCompiledPixelShader = nullptr;
			hResult = D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pCompiledPixelShader, nullptr);

			if (hResult != S_OK || !pCompiledPixelShader) return false;

			hResult = this->_pDevice->CreatePixelShader(pCompiledPixelShader->GetBufferPointer(), pCompiledPixelShader->GetBufferSize(), nullptr, &this->_pPixelShader);

			if (hResult != S_OK || !this->_pPixelShader) return false;

			return true;
		}

		
		BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

		bool Draw::getViewport()
		{
			D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
			UINT viewportCount = sizeof(viewports) / sizeof(D3D11_VIEWPORT);

			this->_pContext->RSGetViewports(&viewportCount, viewports);

			if (viewportCount && viewports[0].Width) {
				this->_viewport = viewports[0];
				this->_pContext->RSSetViewports(1, &this->_viewport);
			}
			else {
				HWND hMainWnd = nullptr;

				EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

				if (!hMainWnd) return false;

				RECT windowRect{};

				if (!GetClientRect(hMainWnd, &windowRect)) return false;

				this->_viewport.Width = static_cast<FLOAT>(windowRect.right);
				this->_viewport.Height = static_cast<FLOAT>(windowRect.bottom);
				this->_viewport.TopLeftX = static_cast<FLOAT>(windowRect.left);
				this->_viewport.TopLeftY = static_cast<FLOAT>(windowRect.top);
				this->_viewport.MinDepth = 0.0f;
				this->_viewport.MaxDepth = 1.0f;
				this->_pContext->RSSetViewports(1, &this->_viewport);
			}

			return true;
		}


		BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;
			
			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}


		bool Draw::setupOrtho() {
			DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicOffCenterLH(0, this->_viewport.Width, this->_viewport.Height, 0, 0.0f, 1.0f);

			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.ByteWidth = sizeof(ortho);
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pSysMem = &ortho;
			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, &subresourceData, &this->_pConstantBuffer);
			
			if (hResult != S_OK || !this->_pConstantBuffer) return false;

			this->_pContext->VSSetConstantBuffers(0, 1, &this->_pConstantBuffer);
			
			return true;
		}

	}

}