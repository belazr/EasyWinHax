#include "dx9Draw.h"

namespace hax {

	namespace dx9 {

		Draw::Draw() : iWindowWidth(0), iWindowHeight(0), fWindowWidth(0.), fWindowHeight(0.), pDevice(nullptr), pFont(nullptr) {}


		void Draw::drawTriangleStrip(const Vector2 corners[], UINT count, rgb::Color color) const {

			if (!this->pDevice) return;

			Vertex* const data = new Vertex[count];

			if (!data) return;

			for (UINT i = 0; i < count; i++) {
				data[i].x = corners[i].x;
				data[i].y = corners[i].y;
				data[i].z = 1.f;
				data[i].rhw = 1.f;
				data[i].color = color;
			}

			this->pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, count, data, sizeof(Vertex));
			delete[] data;

			return;
		}


		void Draw::drawString(const Vector2* pos, const char* text, rgb::Color color) const {

			if (!this->pDevice || !this->pFont) return;

			const size_t size = strlen(text);

			for (size_t indexInString = 0; indexInString < size; indexInString++) {
				const Fontchar* pCurChar = nullptr;

				switch (text[indexInString]) {
				case '!':
					pCurChar = &pFont->pCharset->exclamationPoint;
					break;
				case '"':
					pCurChar = &pFont->pCharset->doubleQuotes;
					break;
				case '#':
					pCurChar = &pFont->pCharset->poundSign;
					break;
				case '$':
					pCurChar = &pFont->pCharset->dollarSign;
					break;
				case '%':
					pCurChar = &pFont->pCharset->percentSign;
					break;
				case '&':
					pCurChar = &pFont->pCharset->ampersand;
					break;
				case '\'':
					pCurChar = &pFont->pCharset->singleQuote;
					break;
				case '(':
					pCurChar = &pFont->pCharset->openParanthesis;
					break;
				case ')':
					pCurChar = &pFont->pCharset->closeParanthesis;
					break;
				case '*':
					pCurChar = &pFont->pCharset->asterisk;
					break;
				case '+':
					pCurChar = &pFont->pCharset->plus;
					break;
				case ',':
					pCurChar = &pFont->pCharset->comma;
					break;
				case '-':
					pCurChar = &pFont->pCharset->dash;
					break;
				case '.':
					pCurChar = &pFont->pCharset->period;
					break;
				case '/':
					pCurChar = &pFont->pCharset->slash;
					break;
				case '0':
					pCurChar = &pFont->pCharset->zero;
					break;
				case '1':
					pCurChar = &pFont->pCharset->one;
					break;
				case '2':
					pCurChar = &pFont->pCharset->two;
					break;
				case '3':
					pCurChar = &pFont->pCharset->three;
					break;
				case '4':
					pCurChar = &pFont->pCharset->four;
					break;
				case '5':
					pCurChar = &pFont->pCharset->five;
					break;
				case '6':
					pCurChar = &pFont->pCharset->six;
					break;
				case '7':
					pCurChar = &pFont->pCharset->seven;
					break;
				case '8':
					pCurChar = &pFont->pCharset->eight;
					break;
				case '9':
					pCurChar = &pFont->pCharset->nine;
					break;
				case ':':
					pCurChar = &pFont->pCharset->colon;
					break;
				case ';':
					pCurChar = &pFont->pCharset->semicolon;
					break;
				case '<':
					pCurChar = &pFont->pCharset->smaller;
					break;
				case '=':
					pCurChar = &pFont->pCharset->equals;
					break;
				case '>':
					pCurChar = &pFont->pCharset->greater;
					break;
				case '?':
					pCurChar = &pFont->pCharset->questionmark;
					break;
				case '@':
					pCurChar = &pFont->pCharset->atSign;
					break;
				case 'A':
					pCurChar = &pFont->pCharset->upperA;
					break;
				case 'B':
					pCurChar = &pFont->pCharset->upperB;
					break;
				case 'C':
					pCurChar = &pFont->pCharset->upperC;
					break;
				case 'D':
					pCurChar = &pFont->pCharset->upperD;
					break;
				case 'E':
					pCurChar = &pFont->pCharset->upperE;
					break;
				case 'F':
					pCurChar = &pFont->pCharset->upperF;
					break;
				case 'G':
					pCurChar = &pFont->pCharset->upperG;
					break;
				case 'H':
					pCurChar = &pFont->pCharset->upperH;
					break;
				case 'I':
					pCurChar = &pFont->pCharset->upperI;
					break;
				case 'J':
					pCurChar = &pFont->pCharset->upperJ;
					break;
				case 'K':
					pCurChar = &pFont->pCharset->upperK;
					break;
				case 'L':
					pCurChar = &pFont->pCharset->upperL;
					break;
				case 'M':
					pCurChar = &pFont->pCharset->upperM;
					break;
				case 'N':
					pCurChar = &pFont->pCharset->upperN;
					break;
				case 'O':
					pCurChar = &pFont->pCharset->upperO;
					break;
				case 'P':
					pCurChar = &pFont->pCharset->upperP;
					break;
				case 'Q':
					pCurChar = &pFont->pCharset->upperQ;
					break;
				case 'R':
					pCurChar = &pFont->pCharset->upperR;
					break;
				case 'S':
					pCurChar = &pFont->pCharset->upperS;
					break;
				case 'T':
					pCurChar = &pFont->pCharset->upperT;
					break;
				case 'U':
					pCurChar = &pFont->pCharset->upperU;
					break;
				case 'V':
					pCurChar = &pFont->pCharset->upperV;
					break;
				case 'W':
					pCurChar = &pFont->pCharset->upperW;
					break;
				case 'X':
					pCurChar = &pFont->pCharset->upperX;
					break;
				case 'Y':
					pCurChar = &pFont->pCharset->upperY;
					break;
				case 'Z':
					pCurChar = &pFont->pCharset->upperZ;
					break;
				case '[':
					pCurChar = &pFont->pCharset->openBracket;
					break;
				case '\\':
					pCurChar = &pFont->pCharset->backslash;
					break;
				case ']':
					pCurChar = &pFont->pCharset->closeBracket;
					break;
				case '^':
					pCurChar = &pFont->pCharset->circumflex;
					break;
				case '_':
					pCurChar = &pFont->pCharset->underscore;
					break;
				case '`':
					pCurChar = &pFont->pCharset->backtick;
					break;
				case 'a':
					pCurChar = &pFont->pCharset->lowerA;
					break;
				case 'b':
					pCurChar = &pFont->pCharset->lowerB;
					break;
				case 'c':
					pCurChar = &pFont->pCharset->lowerC;
					break;
				case 'd':
					pCurChar = &pFont->pCharset->lowerD;
					break;
				case 'e':
					pCurChar = &pFont->pCharset->lowerE;
					break;
				case 'f':
					pCurChar = &pFont->pCharset->lowerF;
					break;
				case 'g':
					pCurChar = &pFont->pCharset->lowerG;
					break;
				case 'h':
					pCurChar = &pFont->pCharset->lowerH;
					break;
				case 'i':
					pCurChar = &pFont->pCharset->lowerI;
					break;
				case 'j':
					pCurChar = &pFont->pCharset->lowerJ;
					break;
				case 'k':
					pCurChar = &pFont->pCharset->lowerK;
					break;
				case 'l':
					pCurChar = &pFont->pCharset->lowerL;
					break;
				case 'm':
					pCurChar = &pFont->pCharset->lowerM;
					break;
				case 'n':
					pCurChar = &pFont->pCharset->lowerN;
					break;
				case 'o':
					pCurChar = &pFont->pCharset->lowerO;
					break;
				case 'p':
					pCurChar = &pFont->pCharset->lowerP;
					break;
				case 'q':
					pCurChar = &pFont->pCharset->lowerQ;
					break;
				case 'r':
					pCurChar = &pFont->pCharset->lowerR;
					break;
				case 's':
					pCurChar = &pFont->pCharset->lowerS;
					break;
				case 't':
					pCurChar = &pFont->pCharset->lowerT;
					break;
				case 'u':
					pCurChar = &pFont->pCharset->lowerU;
					break;
				case 'v':
					pCurChar = &pFont->pCharset->lowerV;
					break;
				case 'w':
					pCurChar = &pFont->pCharset->lowerW;
					break;
				case 'x':
					pCurChar = &pFont->pCharset->lowerX;
					break;
				case 'y':
					pCurChar = &pFont->pCharset->lowerY;
					break;
				case 'z':
					pCurChar = &pFont->pCharset->lowerZ;
					break;
				case '{':
					pCurChar = &pFont->pCharset->openBrace;
					break;
				case '|':
					pCurChar = &pFont->pCharset->pipe;
					break;
				case '}':
					pCurChar = &pFont->pCharset->closeBrace;
					break;
				case '~':
					pCurChar = &pFont->pCharset->tilde;
					break;
				case '°':
					pCurChar = &pFont->pCharset->degrees;
					break;
				}

				if (pCurChar && pCurChar->pixel) {
					// the index of the character in the charset array of the class is the same as the offset in the charset structure
					// bit of a hack
					const size_t indexInCharset = pCurChar - &pFont->pCharset->exclamationPoint;
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pFont->pCharset->width + 1) * indexInString, pos->y - pFont->pCharset->height };
					this->drawFontchar(pCurChar, &curPos, indexInCharset, color);
				}

			}

			return;
		}


		void Draw::beginDraw(IDirect3DDevice9* pOriginalDevice) {

			if (!this->pDevice) {
				this->pDevice = pOriginalDevice;
			}

			pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

			return;
		}


		bool Draw::getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
			HWND hWnd = nullptr;
			EnumWindows(getWindowHandleCallback, reinterpret_cast<LPARAM>(&hWnd));

			if (!hWnd) return false;

			RECT wndRect{};

			if (!GetWindowRect(hWnd, &wndRect)) return false;

			setWindowSize(wndRect.right - wndRect.left, wndRect.bottom - wndRect.top);

			IDirect3D9* const pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);

			if (!pDirect3D9) return false;

			D3DPRESENT_PARAMETERS d3dpp{};
			d3dpp.hDeviceWindow = hWnd;
			d3dpp.Windowed = FALSE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

			IDirect3DDevice9* pDirect3D9Device = nullptr;

			HRESULT hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

			if (hResult != S_OK) {
				d3dpp.Windowed = TRUE;
				hResult = pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDirect3D9Device);

				if (hResult != S_OK) {
					pDirect3D9->Release();

					return false;
				}

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


		void Draw::setWindowSize(int width, int height) {
			this->iWindowWidth = width;
			this->iWindowHeight = height;
			this->fWindowWidth = static_cast<float>(width);
			this->fWindowHeight = static_cast<float>(height);

			return;
		}


		HRESULT Draw::drawFontchar(const Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const {

			if (!this->pDevice || !this->pFont) return E_FAIL;

			// vertex array is allocated at first use
			// array is not deleted during object lifetime for (meassurable) performance reasons
			if (!pFont->charVertexArrays[index]) {
				pFont->charVertexArrays[index] = new Vertex[pChar->pixelCount]{};
			}

			if (pFont->charVertexArrays[index]) {

				for (unsigned int i = 0; i < pChar->pixelCount; i++) {
					pFont->charVertexArrays[index][i].x = pos->x + pChar->pixel[i].x;
					pFont->charVertexArrays[index][i].y = pos->y + pChar->pixel[i].y;
					pFont->charVertexArrays[index][i].z = 1.0f;
					pFont->charVertexArrays[index][i].rhw = 1.0f;
					pFont->charVertexArrays[index][i].color = color;
				}

				return pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, pChar->pixelCount, pFont->charVertexArrays[index], sizeof(Vertex));
			}

			return E_FAIL;
		}


		BOOL CALLBACK getWindowHandleCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}

	}

}

