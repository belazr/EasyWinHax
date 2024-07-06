#include "dx11Draw.h"
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
			_pointListBufferData{}, _triangleListBufferData{}, _viewport{}, _isInit{}, _isBegin{} {}


		Draw::~Draw() {

			if (this->_pRenderTargetView) {
				this->_pRenderTargetView->Release();
			}

			if (this->_pConstantBuffer) {
				this->_pConstantBuffer->Release();
			}

			if (this->_pContext) {
				this->destroyBufferData(&this->_triangleListBufferData);
				this->destroyBufferData(&this->_pointListBufferData);
			}

			if (this->_pPixelShader) {
				this->_pPixelShader->Release();
			}

			if (this->_pVertexLayout) {
				this->_pVertexLayout->Release();
			}

			if (this->_pVertexShader) {
				this->_pVertexShader->Release();
			}

			if (this->_pContext) {
				this->_pContext->Release();
			}

			if (this->_pDevice) {
				this->_pDevice->Release();
			}

		}


		void Draw::beginDraw(Engine* pEngine) {
			this->_isBegin = false;

			IDXGISwapChain* const pSwapChain = reinterpret_cast<IDXGISwapChain*>(pEngine->pHookArg1);

			if (!this->_isInit) {
				this->_isInit = this->initialize(pSwapChain);
			}

			D3D11_VIEWPORT curViewport{};
			this->getCurrentViewport(&curViewport);

			if (curViewport.Width != this->_viewport.Width || curViewport.Height != this->_viewport.Height) {
				this->_viewport = curViewport;
				
				if (!this->updateConstantBuffer()) return;
				
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}

			if (!this->_pRenderTargetView) {
				// the render target view is released every frame in endDraw() and there is no leftover reference to the backbuffer
				// so it has to be acquired every frame as well
				// this is done so resolution changes do not break rendering
				ID3D11Texture2D* pBackBuffer = nullptr;
				pSwapChain->GetBuffer(0u, IID_PPV_ARGS(&pBackBuffer));

				if (!pBackBuffer) return;

				if (FAILED(this->_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &this->_pRenderTargetView))) return;

				pBackBuffer->Release();
			}

			if (!this->mapBufferData(&this->_pointListBufferData)) return;
			
			if (!this->mapBufferData(&this->_triangleListBufferData)) return;

			this->_pContext->OMSetRenderTargets(1u, &this->_pRenderTargetView, nullptr);
			this->_pContext->VSSetConstantBuffers(0u, 1u, &this->_pConstantBuffer);
			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0u);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0u);

			this->_isBegin = true;

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isBegin) return;

			this->drawBufferData(&this->_triangleListBufferData, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			this->drawBufferData(&this->_pointListBufferData, D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

			if (this->_pRenderTargetView) {
				this->_pRenderTargetView->Release();
				this->_pRenderTargetView = nullptr;
			}
			
			return;
		}


		void Draw::drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) {

			if (!this->_isBegin || count % 3u) return;
			
			this->copyToBufferData(&this->_triangleListBufferData, corners, count, color);

			return;
		}


		void Draw::drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset) {

			if (!this->_isBegin) return;

			this->copyToBufferData(&this->_pointListBufferData, coordinates, count, color, offset);

			return;
		}


		constexpr uint32_t INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT = 1000u;
		constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT = 99u;

		bool Draw::initialize(IDXGISwapChain* pSwapChain) {
			const HRESULT hResult = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&this->_pDevice));

			if (hResult != S_OK || !this->_pDevice) return false;

			if (!this->_pContext) {
				this->_pDevice->GetImmediateContext(&this->_pContext);
			}

			if (!this->_pContext) return false;
			
			if (!this->compileShaders()) return false;

			this->destroyBufferData(&this->_pointListBufferData);

			if (!this->createBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_VERTEX_COUNT)) return false;

			this->destroyBufferData(&this->_triangleListBufferData);

			if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_VERTEX_COUNT)) return false;

			if (!this->_pConstantBuffer) {
				
				if (!this->createConstantBuffer()) return false;

			}
			
			return true;
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
			ID3D10Blob* pCompiledVertexShader{};
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

			ID3D10Blob* pCompiledPixelShader{};
			hResult = D3DCompile(shader, sizeof(shader), nullptr, nullptr, nullptr, "mainPS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0u, &pCompiledPixelShader, nullptr);

			if (hResult != S_OK || !pCompiledPixelShader) return false;

			hResult = this->_pDevice->CreatePixelShader(pCompiledPixelShader->GetBufferPointer(), pCompiledPixelShader->GetBufferSize(), nullptr, &this->_pPixelShader);

			pCompiledPixelShader->Release();

			if (hResult != S_OK || !this->_pPixelShader) return false;

			return true;
		}


		static BOOL CALLBACK getMainWindowCallback(HWND hWnd, LPARAM lParam);

		void Draw::getCurrentViewport(D3D11_VIEWPORT* pViewport) const {
			D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
			UINT viewportCount = sizeof(viewports) / sizeof(D3D11_VIEWPORT);

			this->_pContext->RSGetViewports(&viewportCount, viewports);

			*pViewport = viewports[0];

			if (!viewportCount || !pViewport->Width) {
				HWND hMainWnd = nullptr;

				EnumWindows(getMainWindowCallback, reinterpret_cast<LPARAM>(&hMainWnd));

				if (!hMainWnd) return;

				RECT windowRect{};

				if (!GetClientRect(hMainWnd, &windowRect)) return;

				pViewport->Width = static_cast<FLOAT>(windowRect.right);
				pViewport->Height = static_cast<FLOAT>(windowRect.bottom);
				pViewport->TopLeftX = static_cast<FLOAT>(windowRect.left);
				pViewport->TopLeftY = static_cast<FLOAT>(windowRect.top);
				pViewport->MinDepth = 0.f;
				pViewport->MaxDepth = 1.f;
				this->_pContext->RSSetViewports(1u, pViewport);
			}

			return;
		}


		bool Draw::createConstantBuffer() {
			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bufferDesc.ByteWidth = 16 * sizeof(float);
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &this->_pConstantBuffer);

			if (hResult != S_OK || !this->_pConstantBuffer) return false;

			return true;
		}


		bool Draw::createBufferData(BufferData* pBufferData, uint32_t vertexCount) const {
			RtlSecureZeroMemory(pBufferData, sizeof(BufferData));
			
			D3D11_BUFFER_DESC bufferDesc{};
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.ByteWidth = vertexCount * sizeof(Vertex);
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, nullptr, &pBufferData->pVertexBuffer);

			if (hResult != S_OK || !pBufferData->pVertexBuffer) return false;

			pBufferData->vertexBufferSize = bufferDesc.ByteWidth;

			return true;
		}


		void Draw::destroyBufferData(BufferData* pBufferData) const {
			
			if (pBufferData->pVertexBuffer) {
				this->_pContext->Unmap(pBufferData->pVertexBuffer, 0u);
				pBufferData->pLocalVertexBuffer = nullptr;
				pBufferData->pVertexBuffer->Release();
				pBufferData->pVertexBuffer = nullptr;
			}
			
			pBufferData->vertexBufferSize = 0u;
			pBufferData->curOffset = 0u;

			return;
		}


		bool Draw::updateConstantBuffer() const {
			const float viewLeft = this->_viewport.TopLeftX;
			const float viewRight = this->_viewport.TopLeftX + this->_viewport.Width;
			const float viewTop = this->_viewport.TopLeftY;
			const float viewBottom = this->_viewport.TopLeftY + this->_viewport.Height;

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


		bool Draw::mapBufferData(BufferData* pBufferData) const {
			
			if (!pBufferData->pVertexBuffer) return false;
			
			if (!pBufferData->pLocalVertexBuffer) {
				D3D11_MAPPED_SUBRESOURCE subresourcePoints{};
				
				if (FAILED(this->_pContext->Map(pBufferData->pVertexBuffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &subresourcePoints))) return false;
				
				pBufferData->pLocalVertexBuffer = reinterpret_cast<Vertex*>(subresourcePoints.pData);
			}
			
			return pBufferData->pLocalVertexBuffer != nullptr;
		}


		void Draw::copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset) const {
			
			if (!pBufferData->pLocalVertexBuffer) return;

			const uint32_t newVertexCount = pBufferData->curOffset + count;

			if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize) {

				if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

			}

			for (uint32_t i = 0u; i < count; i++) {
				const Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
				memcpy(&(pBufferData->pLocalVertexBuffer[pBufferData->curOffset + i]), &curVertex, sizeof(Vertex));
			}

			pBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const {

			if (newVertexCount <= pBufferData->curOffset) return true;

			BufferData oldBufferData = *pBufferData;

			if (!this->createBufferData(pBufferData, newVertexCount)) {
				this->destroyBufferData(pBufferData);

				return false;
			}

			if (!this->mapBufferData(pBufferData)) return false;


			if (oldBufferData.pLocalVertexBuffer) {
				memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, oldBufferData.vertexBufferSize);
			}

			pBufferData->curOffset = oldBufferData.curOffset;

			this->destroyBufferData(&oldBufferData);

			return true;
		}


		void Draw::drawBufferData(BufferData* pBufferData, D3D11_PRIMITIVE_TOPOLOGY topology) const {
			D3D11_PRIMITIVE_TOPOLOGY curTopology{};
			this->_pContext->IAGetPrimitiveTopology(&curTopology);

			if (!curTopology) return;

			this->_pContext->Unmap(pBufferData->pVertexBuffer, 0u);
			pBufferData->pLocalVertexBuffer = nullptr;

			constexpr UINT STRIDE = sizeof(Vertex);
			constexpr UINT OFFSET = 0u;
			this->_pContext->IASetVertexBuffers(0u, 1u, &pBufferData->pVertexBuffer, &STRIDE, &OFFSET);
			this->_pContext->IASetPrimitiveTopology(topology);
			this->_pContext->Draw(pBufferData->curOffset, 0u);
			this->_pContext->IASetPrimitiveTopology(curTopology);

			pBufferData->curOffset = 0u;

			return;
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