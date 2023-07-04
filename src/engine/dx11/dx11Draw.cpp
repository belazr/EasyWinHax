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
			_pDevice(nullptr), _pContext(nullptr), _pVertexShader(nullptr), _pVertexLayout(nullptr), _pPixelShader(nullptr), _pConstantBuffer(nullptr), _originalTopology{}
		{}


		Draw::~Draw() {
			ULONG ret = 0;

			if (this->_pDevice) {
				ret = this->_pDevice->Release();
			}

			if (this->_pContext) {
				ret = this->_pContext->Release();
			}

			if (this->_pVertexShader) {
				ret = this->_pVertexShader->Release();
			}

			if (this->_pVertexLayout) {
				ret = this->_pVertexLayout->Release();
			}

			if (this->_pDevice) {
				ret = this->_pDevice->Release();
			}

			if (this->_pPixelShader) {
				ret = this->_pPixelShader->Release();
			}

			if (this->_pConstantBuffer) {
				ret = this->_pConstantBuffer->Release();
			}

		}


		void Draw::beginDraw(const Engine* pEngine) {

			if (!this->_originalTopology) {
				IDXGISwapChain* const pSwapChain = reinterpret_cast<IDXGISwapChain*>(const_cast<hax::Engine*>(pEngine));
				const HRESULT hResult = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&this->_pDevice));

				if (hResult != S_OK || !this->_pDevice) return;

				this->_pDevice->GetImmediateContext(&this->_pContext);

				if (!this->_pContext) return;

				if (!this->compileShaders()) return;

				this->_pContext->IAGetPrimitiveTopology(&this->_originalTopology);

			}

			setupConstantBuffer();

			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const {
			UNREFERENCED_PARAMETER(pEngine);

			this->_pContext->IASetPrimitiveTopology(this->_originalTopology);

			return;
		}


		typedef struct Vertex {
			float x, y, z;
			D3DCOLORVALUE color;
		}Vertex;

		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {

			if (!this->_pDevice || !this->_pContext) return;

			Vertex* const data = new Vertex[count];

			if (!data) return;

			for (UINT i = 0; i < count; i++) {
				data[i].x = corners[i].x;
				data[i].y = corners[i].y;
				data[i].z = 1.f;
				data[i].color.r = FLOAT_R(color);
				data[i].color.g = FLOAT_G(color);
				data[i].color.b = FLOAT_B(color);
				data[i].color.a = FLOAT_A(color);
			}

			D3D11_BUFFER_DESC bufferDescLine{};
			bufferDescLine.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bufferDescLine.ByteWidth = count * sizeof(Vertex);
			bufferDescLine.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pSysMem = data;

			ID3D11Buffer* pVertexBuffer = nullptr;

			this->_pDevice->CreateBuffer(&bufferDescLine, &subresourceData, &pVertexBuffer);

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			this->_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
			this->_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

			this->_pContext->Draw(count, 0);

			pVertexBuffer->Release();
			delete[] data;

			return;
		}


		void Draw::drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const {

			return;
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
				psi.pos = mul( projection, pos );
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


			if (hResult != S_OK || !this->_pVertexShader) {
				pCompiledVertexShader->Release();

				return false;
			}

			D3D11_INPUT_ELEMENT_DESC inputElementDesc[2]{
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
			};

			hResult = this->_pDevice->CreateInputLayout(inputElementDesc, sizeof(inputElementDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), &this->_pVertexLayout);

			pCompiledVertexShader->Release();

			if (hResult != S_OK) return false;

			ID3D10Blob* pCompiledPixelShader = nullptr;
			hResult = D3DCompile(shader, strlen(shader), 0, nullptr, nullptr, "PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pCompiledPixelShader, nullptr);

			if (hResult != S_OK || !pCompiledPixelShader) return false;

			hResult = this->_pDevice->CreatePixelShader(pCompiledPixelShader->GetBufferPointer(), pCompiledPixelShader->GetBufferSize(), nullptr, &this->_pPixelShader);

			pCompiledPixelShader->Release();

			if (hResult != S_OK || !this->_pPixelShader) return false;

			return true;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

		void Draw::setupConstantBuffer() {
			D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
			UINT viewportCount = sizeof(viewports) / sizeof(D3D11_VIEWPORT);

			this->_pContext->RSGetViewports(&viewportCount, viewports);

			D3D11_VIEWPORT viewport = viewports[0];

			if (!viewportCount || !viewport.Width) {
				HWND hMainWnd = nullptr;

				EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

				if (!hMainWnd) return;

				RECT windowRect{};

				if (!GetClientRect(hMainWnd, &windowRect)) return;

				viewport.Width = static_cast<FLOAT>(windowRect.right);
				viewport.Height = static_cast<FLOAT>(windowRect.bottom);
				viewport.TopLeftX = static_cast<FLOAT>(windowRect.left);
				viewport.TopLeftY = static_cast<FLOAT>(windowRect.top);
				viewport.MinDepth = 0.0f;
				viewport.MaxDepth = 1.0f;
				this->_pContext->RSSetViewports(1, &viewport);
			}

			DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicOffCenterLH(0, viewport.Width, viewport.Height, 0, 0.0f, 1.0f);

			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.ByteWidth = sizeof(ortho);
			bufferDesc.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pSysMem = &ortho;

			if (this->_pConstantBuffer) {
				this->_pConstantBuffer->Release();
			}

			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, &subresourceData, &this->_pConstantBuffer);

			if (hResult != S_OK || !this->_pConstantBuffer) return;

			this->_pContext->VSSetConstantBuffers(0, 1, &this->_pConstantBuffer);

			return;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}

	}

}