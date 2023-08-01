#include "dx11Draw.h"
#include "dx11Vertex.h"
#include "..\..\Engine.h"
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
			_pDevice{}, _pContext{}, _pVertexShader{}, _pVertexLayout{}, _pPixelShader{}, _pConstantBuffer{},
			_pVertexBuffer{}, _vertexBufferSize{}, _viewport {}, _originalTopology{}, _isInit{} {}


		Draw::~Draw() {

			if (this->_pDevice) {
				this->_pDevice->Release();
			}

			if (this->_pContext) {
				this->_pContext->Release();
			}

			if (this->_pVertexShader) {
				this->_pVertexShader->Release();
			}

			if (this->_pVertexLayout) {
				this->_pVertexLayout->Release();
			}

			if (this->_pPixelShader) {
				this->_pPixelShader->Release();
			}

			if (this->_pConstantBuffer) {
				this->_pConstantBuffer->Release();
			}

			if (this->_pVertexBuffer) {
				this->_pVertexBuffer->Release();
			}

		}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				IDXGISwapChain* const pSwapChain = reinterpret_cast<IDXGISwapChain*>(pEngine->pHookArg);
				const HRESULT hResult = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&this->_pDevice));

				if (hResult != S_OK || !this->_pDevice) return;

				this->_pDevice->GetImmediateContext(&this->_pContext);

				if (!this->_pContext) return;

				if (!this->compileShaders()) return;

				this->_pContext->IAGetPrimitiveTopology(&this->_originalTopology);
				this->_isInit = true;
			}

			if (createConstantBuffer()) {
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}

			this->_pContext->VSSetConstantBuffers(0, 1, &this->_pConstantBuffer);
			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			UNREFERENCED_PARAMETER(pEngine);

			if (this->_originalTopology) {
				this->_pContext->IASetPrimitiveTopology(this->_originalTopology);
			}
			
			return;
		}


		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit) return;
			
			if (!this->setupVertexBuffer(corners, count, color)) return;

			this->_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
			this->_pContext->Draw(count, 0);

			return;
		}


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {
			
			if (!this->_isInit || !pFont) return;

			const dx::Font* const pDxFont = reinterpret_cast<const dx::Font*>(pFont);

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;

				const dx::CharIndex index = dx::charToCharIndex(c);
				const dx::Fontchar* pCurChar = &pDxFont->chars[index];

				if (pCurChar && pCurChar->pixel) {
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pDxFont->width + 1) * i, pos->y - pDxFont->height };
					this->drawFontchar(pCurChar, &curPos, color);
				}

			}

			return;
		}


		constexpr const char shader[]{ R"(
			cbuffer vertexBuffer : register(b0) {
				float4x4 projectionMatrix;
			}

			struct VSI {
			    float2 pos : POSITION;
				float4 col : COLOR0;
			};

			struct PSI {
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
			};

			PSI mainVS(VSI input) {
				PSI output;
				output.pos = mul(projectionMatrix, float4(input.pos.xy, 0.f, 1.f));
				output.col = input.col;
				return output;
			}

			float4 mainPS(PSI input) : SV_TARGET {
				return input.col;
			}
		)" };

		bool Draw::compileShaders() {
			ID3D10Blob* pCompiledVertexShader = nullptr;
			HRESULT hResult = D3DCompile(shader, sizeof(shader), nullptr, nullptr, nullptr, "mainVS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pCompiledVertexShader, nullptr);

			if (hResult != S_OK || !pCompiledVertexShader) return false;

			hResult = this->_pDevice->CreateVertexShader(pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), nullptr, &this->_pVertexShader);

			if (hResult != S_OK || !this->_pVertexShader) {
				pCompiledVertexShader->Release();

				return false;
			}

			D3D11_INPUT_ELEMENT_DESC inputElementDesc[2]{
				{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
			};

			hResult = this->_pDevice->CreateInputLayout(inputElementDesc, sizeof(inputElementDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), &this->_pVertexLayout);

			pCompiledVertexShader->Release();

			if (hResult != S_OK) return false;

			ID3D10Blob* pCompiledPixelShader = nullptr;
			hResult = D3DCompile(shader, sizeof(shader), 0, nullptr, nullptr, "mainPS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &pCompiledPixelShader, nullptr);

			if (hResult != S_OK || !pCompiledPixelShader) return false;

			hResult = this->_pDevice->CreatePixelShader(pCompiledPixelShader->GetBufferPointer(), pCompiledPixelShader->GetBufferSize(), nullptr, &this->_pPixelShader);

			pCompiledPixelShader->Release();

			if (hResult != S_OK || !this->_pPixelShader) return false;

			return true;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

		bool Draw::createConstantBuffer() {
			D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
			UINT viewportCount = sizeof(viewports) / sizeof(D3D11_VIEWPORT);

			this->_pContext->RSGetViewports(&viewportCount, viewports);

			D3D11_VIEWPORT curViewport = viewports[0];

			if (!viewportCount || !curViewport.Width) {
				HWND hMainWnd = nullptr;

				EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

				if (!hMainWnd) return false;

				RECT windowRect{};

				if (!GetClientRect(hMainWnd, &windowRect)) return false;

				curViewport.Width = static_cast<FLOAT>(windowRect.right);
				curViewport.Height = static_cast<FLOAT>(windowRect.bottom);
				curViewport.TopLeftX = static_cast<FLOAT>(windowRect.left);
				curViewport.TopLeftY = static_cast<FLOAT>(windowRect.top);
				curViewport.MinDepth = 0.0f;
				curViewport.MaxDepth = 1.0f;
				this->_pContext->RSSetViewports(1, &curViewport);
			}

			if (this->_viewport.Width == curViewport.Width && this->_viewport.Height == curViewport.Height) return false;

			DirectX::XMMATRIX ortho = DirectX::XMMatrixOrthographicOffCenterLH(0, curViewport.Width, curViewport.Height, 0, 0.f, 1.f);

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

			if (hResult != S_OK || !this->_pConstantBuffer) return false;

			this->_viewport = curViewport;

			return true;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;

			char className[MAX_PATH]{};
			
			if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

			if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}

		void Draw::drawFontchar(const dx::Fontchar* pChar, const Vector2* pos, rgb::Color color) {
			
			if (!this->_isInit) return;

			if (!this->setupVertexBuffer(pChar->pixel, pChar->pixelCount, color, *pos)) return;

			this->_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
			this->_pContext->Draw(pChar->pixelCount, 0);	

			return; 
		}

		bool Draw::setupVertexBuffer(const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) {
			const UINT bytesNeeded = count * sizeof(Vertex);
			
			if (!this->_pVertexBuffer || this->_vertexBufferSize < bytesNeeded) {

				if (this->_pVertexBuffer) {
					this->_pVertexBuffer->Release();
				}

				D3D11_BUFFER_DESC bufferDesc{};
				bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				bufferDesc.ByteWidth = bytesNeeded;
				bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
				bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

				const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &this->_pVertexBuffer);

				if (hResult != S_OK || !this->_pVertexBuffer) {
					this->_vertexBufferSize = 0;
					
					return false;
				}

				this->_vertexBufferSize = bytesNeeded;
			}

			D3D11_MAPPED_SUBRESOURCE subresource{};
			const HRESULT hResult = this->_pContext->Map(this->_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

			if (hResult != S_OK) {
				this->_vertexBufferSize = 0;

				return false;
			}

			for (UINT i = 0; i < count; i++) {
				Vertex curVertex({ data[i].x + offset.x, data[i].y + offset.y }, color);
				memcpy(&(reinterpret_cast<Vertex*>(subresource.pData)[i]), &curVertex, sizeof(Vertex));
			}

			this->_pContext->Unmap(this->_pVertexBuffer, 0);

			constexpr UINT STRIDE = sizeof(Vertex);
			constexpr UINT OFFSET = 0;

			this->_pContext->IASetVertexBuffers(0, 1, &this->_pVertexBuffer, &STRIDE, &OFFSET);

			return true;
		}

	}

}