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


		Draw::Draw() : _pDevice{ nullptr }, _pContext{ nullptr }, _pVertexShader{ nullptr }, _pVertexLayout{ nullptr }, _pPixelShader{ nullptr }, _pConstantBuffer{ nullptr }, _viewport{}, _originalTopology{}, _isInit{} {}


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

			if (createConstantBuffer(&this->_pConstantBuffer)) {
				pEngine->setWindowSize(this->_viewport.Width, this->_viewport.Height);
			}

			this->_pContext->VSSetConstantBuffers(0, 1, &this->_pConstantBuffer);
			this->_pContext->VSSetShader(this->_pVertexShader, nullptr, 0);
			this->_pContext->IASetInputLayout(this->_pVertexLayout);
			this->_pContext->PSSetShader(this->_pPixelShader, nullptr, 0);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const {
			UNREFERENCED_PARAMETER(pEngine);

			if (this->_originalTopology) {
				this->_pContext->IASetPrimitiveTopology(this->_originalTopology);
			}
			
			return;
		}


		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {

			if (!this->_isInit) return;

			Vertex* const data = reinterpret_cast<Vertex*>(new BYTE[count * sizeof(Vertex)]);

			if (!data) return;

			for (UINT i = 0; i < count; i++) {
				data[i] = Vertex{ corners[i], color };
			}

			D3D11_BUFFER_DESC bufferDescLine{};
			bufferDescLine.BindFlags = D3D11_BIND_VERTEX_BUFFER;
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
			
			if (!this->_isInit || !pFont) return;

			dx::Font<Vertex>* const pDx11Font = reinterpret_cast<dx::Font<Vertex>*>(pFont);

			const size_t size = strlen(text);

			for (size_t indexInString = 0; indexInString < size; indexInString++) {
				const dx::Fontchar* pCurChar = nullptr;

				switch (text[indexInString]) {
				case '!':
					pCurChar = &pDx11Font->pCharset->exclamationPoint;
					break;
				case '"':
					pCurChar = &pDx11Font->pCharset->doubleQuotes;
					break;
				case '#':
					pCurChar = &pDx11Font->pCharset->poundSign;
					break;
				case '$':
					pCurChar = &pDx11Font->pCharset->dollarSign;
					break;
				case '%':
					pCurChar = &pDx11Font->pCharset->percentSign;
					break;
				case '&':
					pCurChar = &pDx11Font->pCharset->ampersand;
					break;
				case '\'':
					pCurChar = &pDx11Font->pCharset->singleQuote;
					break;
				case '(':
					pCurChar = &pDx11Font->pCharset->openParanthesis;
					break;
				case ')':
					pCurChar = &pDx11Font->pCharset->closeParanthesis;
					break;
				case '*':
					pCurChar = &pDx11Font->pCharset->asterisk;
					break;
				case '+':
					pCurChar = &pDx11Font->pCharset->plus;
					break;
				case ',':
					pCurChar = &pDx11Font->pCharset->comma;
					break;
				case '-':
					pCurChar = &pDx11Font->pCharset->dash;
					break;
				case '.':
					pCurChar = &pDx11Font->pCharset->period;
					break;
				case '/':
					pCurChar = &pDx11Font->pCharset->slash;
					break;
				case '0':
					pCurChar = &pDx11Font->pCharset->zero;
					break;
				case '1':
					pCurChar = &pDx11Font->pCharset->one;
					break;
				case '2':
					pCurChar = &pDx11Font->pCharset->two;
					break;
				case '3':
					pCurChar = &pDx11Font->pCharset->three;
					break;
				case '4':
					pCurChar = &pDx11Font->pCharset->four;
					break;
				case '5':
					pCurChar = &pDx11Font->pCharset->five;
					break;
				case '6':
					pCurChar = &pDx11Font->pCharset->six;
					break;
				case '7':
					pCurChar = &pDx11Font->pCharset->seven;
					break;
				case '8':
					pCurChar = &pDx11Font->pCharset->eight;
					break;
				case '9':
					pCurChar = &pDx11Font->pCharset->nine;
					break;
				case ':':
					pCurChar = &pDx11Font->pCharset->colon;
					break;
				case ';':
					pCurChar = &pDx11Font->pCharset->semicolon;
					break;
				case '<':
					pCurChar = &pDx11Font->pCharset->smaller;
					break;
				case '=':
					pCurChar = &pDx11Font->pCharset->equals;
					break;
				case '>':
					pCurChar = &pDx11Font->pCharset->greater;
					break;
				case '?':
					pCurChar = &pDx11Font->pCharset->questionmark;
					break;
				case '@':
					pCurChar = &pDx11Font->pCharset->atSign;
					break;
				case 'A':
					pCurChar = &pDx11Font->pCharset->upperA;
					break;
				case 'B':
					pCurChar = &pDx11Font->pCharset->upperB;
					break;
				case 'C':
					pCurChar = &pDx11Font->pCharset->upperC;
					break;
				case 'D':
					pCurChar = &pDx11Font->pCharset->upperD;
					break;
				case 'E':
					pCurChar = &pDx11Font->pCharset->upperE;
					break;
				case 'F':
					pCurChar = &pDx11Font->pCharset->upperF;
					break;
				case 'G':
					pCurChar = &pDx11Font->pCharset->upperG;
					break;
				case 'H':
					pCurChar = &pDx11Font->pCharset->upperH;
					break;
				case 'I':
					pCurChar = &pDx11Font->pCharset->upperI;
					break;
				case 'J':
					pCurChar = &pDx11Font->pCharset->upperJ;
					break;
				case 'K':
					pCurChar = &pDx11Font->pCharset->upperK;
					break;
				case 'L':
					pCurChar = &pDx11Font->pCharset->upperL;
					break;
				case 'M':
					pCurChar = &pDx11Font->pCharset->upperM;
					break;
				case 'N':
					pCurChar = &pDx11Font->pCharset->upperN;
					break;
				case 'O':
					pCurChar = &pDx11Font->pCharset->upperO;
					break;
				case 'P':
					pCurChar = &pDx11Font->pCharset->upperP;
					break;
				case 'Q':
					pCurChar = &pDx11Font->pCharset->upperQ;
					break;
				case 'R':
					pCurChar = &pDx11Font->pCharset->upperR;
					break;
				case 'S':
					pCurChar = &pDx11Font->pCharset->upperS;
					break;
				case 'T':
					pCurChar = &pDx11Font->pCharset->upperT;
					break;
				case 'U':
					pCurChar = &pDx11Font->pCharset->upperU;
					break;
				case 'V':
					pCurChar = &pDx11Font->pCharset->upperV;
					break;
				case 'W':
					pCurChar = &pDx11Font->pCharset->upperW;
					break;
				case 'X':
					pCurChar = &pDx11Font->pCharset->upperX;
					break;
				case 'Y':
					pCurChar = &pDx11Font->pCharset->upperY;
					break;
				case 'Z':
					pCurChar = &pDx11Font->pCharset->upperZ;
					break;
				case '[':
					pCurChar = &pDx11Font->pCharset->openBracket;
					break;
				case '\\':
					pCurChar = &pDx11Font->pCharset->backslash;
					break;
				case ']':
					pCurChar = &pDx11Font->pCharset->closeBracket;
					break;
				case '^':
					pCurChar = &pDx11Font->pCharset->circumflex;
					break;
				case '_':
					pCurChar = &pDx11Font->pCharset->underscore;
					break;
				case '`':
					pCurChar = &pDx11Font->pCharset->backtick;
					break;
				case 'a':
					pCurChar = &pDx11Font->pCharset->lowerA;
					break;
				case 'b':
					pCurChar = &pDx11Font->pCharset->lowerB;
					break;
				case 'c':
					pCurChar = &pDx11Font->pCharset->lowerC;
					break;
				case 'd':
					pCurChar = &pDx11Font->pCharset->lowerD;
					break;
				case 'e':
					pCurChar = &pDx11Font->pCharset->lowerE;
					break;
				case 'f':
					pCurChar = &pDx11Font->pCharset->lowerF;
					break;
				case 'g':
					pCurChar = &pDx11Font->pCharset->lowerG;
					break;
				case 'h':
					pCurChar = &pDx11Font->pCharset->lowerH;
					break;
				case 'i':
					pCurChar = &pDx11Font->pCharset->lowerI;
					break;
				case 'j':
					pCurChar = &pDx11Font->pCharset->lowerJ;
					break;
				case 'k':
					pCurChar = &pDx11Font->pCharset->lowerK;
					break;
				case 'l':
					pCurChar = &pDx11Font->pCharset->lowerL;
					break;
				case 'm':
					pCurChar = &pDx11Font->pCharset->lowerM;
					break;
				case 'n':
					pCurChar = &pDx11Font->pCharset->lowerN;
					break;
				case 'o':
					pCurChar = &pDx11Font->pCharset->lowerO;
					break;
				case 'p':
					pCurChar = &pDx11Font->pCharset->lowerP;
					break;
				case 'q':
					pCurChar = &pDx11Font->pCharset->lowerQ;
					break;
				case 'r':
					pCurChar = &pDx11Font->pCharset->lowerR;
					break;
				case 's':
					pCurChar = &pDx11Font->pCharset->lowerS;
					break;
				case 't':
					pCurChar = &pDx11Font->pCharset->lowerT;
					break;
				case 'u':
					pCurChar = &pDx11Font->pCharset->lowerU;
					break;
				case 'v':
					pCurChar = &pDx11Font->pCharset->lowerV;
					break;
				case 'w':
					pCurChar = &pDx11Font->pCharset->lowerW;
					break;
				case 'x':
					pCurChar = &pDx11Font->pCharset->lowerX;
					break;
				case 'y':
					pCurChar = &pDx11Font->pCharset->lowerY;
					break;
				case 'z':
					pCurChar = &pDx11Font->pCharset->lowerZ;
					break;
				case '{':
					pCurChar = &pDx11Font->pCharset->openBrace;
					break;
				case '|':
					pCurChar = &pDx11Font->pCharset->pipe;
					break;
				case '}':
					pCurChar = &pDx11Font->pCharset->closeBrace;
					break;
				case '~':
					pCurChar = &pDx11Font->pCharset->tilde;
					break;
				case '°':
					pCurChar = &pDx11Font->pCharset->degrees;
					break;
				}

				if (pCurChar && pCurChar->pixel) {
					// the index of the character in the charset array of the class is the same as the offset in the charset structure
					// bit of a hack
					const size_t indexInCharset = pCurChar - &pDx11Font->pCharset->exclamationPoint;
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pDx11Font->pCharset->width + 1) * indexInString, pos->y - pDx11Font->pCharset->height };
					this->drawFontchar(pDx11Font, pCurChar, &curPos, indexInCharset, color);
				}

			}

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

		bool Draw::createConstantBuffer(ID3D11Buffer** ppConstantBuffer) {
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

			if (*ppConstantBuffer) {
				(*ppConstantBuffer)->Release();
			}

			const HRESULT hResult = this->_pDevice->CreateBuffer(&bufferDesc, &subresourceData, ppConstantBuffer);

			if (hResult != S_OK || !*ppConstantBuffer) return false;

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

		void Draw::drawFontchar(dx::Font<Vertex>* pDx11Font, const dx::Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const {
			
			if (!this->_isInit || !pDx11Font) return;

			// vertex array is allocated at first use
			// array is not deleted during object lifetime for (meassurable) performance improvements
			if (!pDx11Font->charVertexArrays[index]) {
				pDx11Font->charVertexArrays[index] = reinterpret_cast<Vertex*>(new BYTE[pChar->pixelCount * sizeof(Vertex)]);
			}

			if (!pDx11Font->charVertexArrays[index]) return;

			for (UINT i = 0; i < pChar->pixelCount; i++) {
				pDx11Font->charVertexArrays[index][i] = Vertex{ pos->x + pChar->pixel[i].x, pos->y + pChar->pixel[i].y, color };
			}

			D3D11_BUFFER_DESC bufferDescLine{};
			bufferDescLine.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDescLine.ByteWidth = pChar->pixelCount * sizeof(Vertex);
			bufferDescLine.Usage = D3D11_USAGE_DEFAULT;

			D3D11_SUBRESOURCE_DATA subresourceData{};
			subresourceData.pSysMem = pDx11Font->charVertexArrays[index];

			ID3D11Buffer* pVertexBuffer = nullptr;
			this->_pDevice->CreateBuffer(&bufferDescLine, &subresourceData, &pVertexBuffer);

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			this->_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
			this->_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

			this->_pContext->Draw(pChar->pixelCount, 0);

			pVertexBuffer->Release();			

			return; 
		}

	}

}