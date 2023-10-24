#include "dx11Draw.h"
#include "dx11Vertex.h"
#include "..\..\Engine.h"
#include <d3dcompiler.h>

namespace hax {

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

			HRESULT hResult = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0u, nullptr, 0u, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, nullptr, nullptr);

			if (hResult != S_OK || !pDevice || !pSwapChain) return false;

			if (memcpy_s(pSwapChainVTable, size, *reinterpret_cast<void**>(pSwapChain), size)) {
				pDevice->Release();
				pSwapChain->Release();

				return false;
			}

			pDevice->Release();
			pSwapChain->Release();

			return true;
		}


		Draw::Draw() :
			_pDevice{}, _pContext{}, _pVertexShader{}, _pVertexLayout{}, _pPixelShader{}, _pConstantBuffer{}, _pRenderTargetView{},
			_pointListBufferData{}, _triangleListBufferData{}, _viewport {}, _originalTopology{}, _isInit{} {}


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

			if (this->_pRenderTargetView) {
				this->_pRenderTargetView->Release();
			}

			if (this->_pointListBufferData.pBuffer) {
				this->_pContext->Unmap(this->_pointListBufferData.pBuffer, 0u);
				this->_pointListBufferData.pBuffer->Release();
			}

			if (this->_triangleListBufferData.pBuffer) {
				this->_pContext->Unmap(this->_triangleListBufferData.pBuffer, 0u);
				this->_triangleListBufferData.pBuffer->Release();
			}

		}


		constexpr UINT INITIAL_POINT_LIST_BUFFER_SIZE = sizeof(Vertex) * 1000;
		constexpr UINT INITIAL_TRIANGLE_LIST_BUFFER_SIZE = sizeof(Vertex) * 100;

		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {
				IDXGISwapChain* const pSwapChain = reinterpret_cast<IDXGISwapChain*>(pEngine->pHookArg);
				const HRESULT hResult = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&this->_pDevice));

				if (hResult != S_OK || !this->_pDevice) return;

				this->_pDevice->GetImmediateContext(&this->_pContext);

				if (!this->_pContext) return;

				if (!this->compileShaders()) return;

				if (!this->createVertexBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return;
				
				if (!this->createVertexBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;

				ID3D11Texture2D* pBackBuffer = nullptr;
				pSwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));

				if (!pBackBuffer) return;

				this->_pDevice->CreateRenderTargetView(pBackBuffer, NULL, &this->_pRenderTargetView);
				pBackBuffer->Release();

				this->_pContext->IAGetPrimitiveTopology(&this->_originalTopology);

				this->_isInit = true;
			}

			if (createConstantBuffer()) {
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}

			this->_pContext->VSSetConstantBuffers(0u, 1u, &this->_pConstantBuffer);
			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0u);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0u);
			this->_pContext->OMSetRenderTargets(1u, &this->_pRenderTargetView, nullptr);

			// mapping the buffers is expensive so it is just done once per frame if no resize is necessary
			D3D11_MAPPED_SUBRESOURCE subresourcePoints{};
			this->_pContext->Map(this->_pointListBufferData.pBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourcePoints);
			this->_pointListBufferData.pLocalBuffer = reinterpret_cast<Vertex*>(subresourcePoints.pData);

			D3D11_MAPPED_SUBRESOURCE subresourceTriangles{};
			this->_pContext->Map(this->_triangleListBufferData.pBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourceTriangles);
			this->_triangleListBufferData.pLocalBuffer = reinterpret_cast<Vertex*>(subresourceTriangles.pData);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			// draw all the buffers at once to save api calls
			this->_pContext->Unmap(this->_pointListBufferData.pBuffer, 0u);
			this->_pointListBufferData.pLocalBuffer = nullptr;
			this->drawVertexBuffer(&this->_pointListBufferData, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

			this->_pContext->Unmap(this->_triangleListBufferData.pBuffer, 0u);
			this->_triangleListBufferData.pLocalBuffer = nullptr;
			this->drawVertexBuffer(&this->_triangleListBufferData, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			if (this->_originalTopology) {
				this->_pContext->IASetPrimitiveTopology(this->_originalTopology);
			}
			
			return;
		}


		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {

			if (!this->_isInit || count % 3u) return;
			
			this->copyToVertexBuffer(&this->_triangleListBufferData, corners, count, color);

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
					// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
					const Vector2 curPos{ pos->x + (pDxFont->width + 2.f) * i, pos->y - pDxFont->height };
					this->copyToVertexBuffer(&this->_pointListBufferData, pCurChar->pixel, pCurChar->pixelCount, color, curPos);
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
			HRESULT hResult = D3DCompile(shader, sizeof(shader), nullptr, nullptr, nullptr, "mainVS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0u, &pCompiledVertexShader, nullptr);

			if (hResult != S_OK || !pCompiledVertexShader) return false;

			hResult = this->_pDevice->CreateVertexShader(pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), nullptr, &this->_pVertexShader);

			if (hResult != S_OK || !this->_pVertexShader) {
				pCompiledVertexShader->Release();

				return false;
			}

			D3D11_INPUT_ELEMENT_DESC inputElementDesc[2]{
				{"POSITION", 0u, DXGI_FORMAT_R32G32_FLOAT, 0u, 0u, D3D11_INPUT_PER_VERTEX_DATA, 0u},
				{"COLOR", 0u, DXGI_FORMAT_R8G8B8A8_UNORM, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0u}
			};

			hResult = this->_pDevice->CreateInputLayout(inputElementDesc, sizeof(inputElementDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), pCompiledVertexShader->GetBufferPointer(), pCompiledVertexShader->GetBufferSize(), &this->_pVertexLayout);

			pCompiledVertexShader->Release();

			if (hResult != S_OK) return false;

			ID3D10Blob* pCompiledPixelShader = nullptr;
			hResult = D3DCompile(shader, sizeof(shader), nullptr, nullptr, nullptr, "mainPS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0u, &pCompiledPixelShader, nullptr);

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
				curViewport.MinDepth = 0.f;
				curViewport.MaxDepth = 1.f;
				this->_pContext->RSSetViewports(1u, &curViewport);
			}

			if (this->_viewport.Width == curViewport.Width && this->_viewport.Height == curViewport.Height) return false;

			const float viewLeft = static_cast<const float>(curViewport.TopLeftX);
			const float viewRight = static_cast<const float>(curViewport.TopLeftX + curViewport.Width);
			const float viewTop = static_cast<const float>(curViewport.TopLeftY);
			const float viewBottom = static_cast<const float>(curViewport.TopLeftY + curViewport.Height);

			const float ortho[][4] {
				{ 2.f / (viewRight - viewLeft), 0.f, 0.f, 0.f  },
				{ 0.f, 2.f / (viewTop - viewBottom), 0.f, 0.f },
				{ 0.f, 0.f, .5f, 0.f },
				{ (viewLeft + viewRight) / (viewLeft - viewRight), (viewTop + viewBottom) / (viewBottom - viewTop), .5f, 1.f }
			};

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
			DWORD processId = 0ul;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId || GetWindow(hWnd, GW_OWNER) || !IsWindowVisible(hWnd)) return TRUE;

			char className[MAX_PATH]{};
			
			if (!GetClassNameA(hWnd, className, MAX_PATH)) return TRUE;

			if (!strcmp(className, "ConsoleWindowClass")) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}


		void Draw::copyToVertexBuffer(VertexBufferData* pVertexBufferData, const Vector2 data[], UINT count, rgb::Color color, Vector2 offset) const {
			const UINT sizeNeeded = (pVertexBufferData->curOffset + count) * sizeof(Vertex);

			if (sizeNeeded > pVertexBufferData->size) {

				if (!this->resizeVertexBuffer(pVertexBufferData, sizeNeeded * 2)) return;

			}

			for (UINT i = 0u; i < count; i++) {
				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
				memcpy(&(pVertexBufferData->pLocalBuffer[pVertexBufferData->curOffset + i]), &curVertex, sizeof(Vertex));
			}

			pVertexBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeVertexBuffer(VertexBufferData* pVertexBufferData, UINT newSize) const {
			const UINT bytesUsed = pVertexBufferData->curOffset * sizeof(Vertex);

			if (newSize < bytesUsed) return false;

			const VertexBufferData oldBufferData = *pVertexBufferData;

			if (!this->createVertexBufferData(pVertexBufferData, newSize)) return false;

			D3D11_MAPPED_SUBRESOURCE subresource{};
			
			if (this->_pContext->Map(pVertexBufferData->pBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresource) != S_OK) return false;

			pVertexBufferData->pLocalBuffer = reinterpret_cast<Vertex*>(subresource.pData);

			if (oldBufferData.pBuffer && oldBufferData.pLocalBuffer) {
				memcpy(pVertexBufferData->pLocalBuffer, oldBufferData.pLocalBuffer, bytesUsed);

				this->_pContext->Unmap(oldBufferData.pBuffer, 0u);
				oldBufferData.pBuffer->Release();
			}

			return true;
		}


		bool Draw::createVertexBufferData(VertexBufferData* pVertexBufferData, UINT size) const {
			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.ByteWidth = size;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &pVertexBufferData->pBuffer);

			if (!pVertexBufferData->pBuffer) {
				pVertexBufferData->pLocalBuffer = nullptr;
				pVertexBufferData->size = 0u;
				pVertexBufferData->curOffset = 0u;

				return false;
			}

			if (hResult != S_OK) return false;

			pVertexBufferData->pLocalBuffer = nullptr;
			pVertexBufferData->size = size;
			pVertexBufferData->curOffset = 0u;

			return true;
		}


		void Draw::drawVertexBuffer(VertexBufferData* pVertexBufferData, D3D11_PRIMITIVE_TOPOLOGY topology) const {
			constexpr UINT STRIDE = sizeof(Vertex);
			constexpr UINT OFFSET = 0u;
			this->_pContext->IASetVertexBuffers(0u, 1u, &pVertexBufferData->pBuffer, &STRIDE, &OFFSET);
			this->_pContext->IASetPrimitiveTopology(topology);
			this->_pContext->Draw(pVertexBufferData->curOffset, 0u);

			pVertexBufferData->curOffset = 0u;

			return;
		}

	}

}