#include "dx9Draw.h"
#include "..\Engine.h"

namespace hax {

	namespace dx9 {

		static BOOL CALLBACK getWindowCallback(HWND hWnd, LPARAM lParam);

		bool getD3D9DeviceVTable(void** pDeviceVTable, size_t size) {
			HWND hWnd = nullptr;
			EnumWindows(getWindowCallback, reinterpret_cast<LPARAM>(&hWnd));

			if (!hWnd) return false;

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


		Draw::Draw() : pDevice(nullptr) {}


		void Draw::beginDraw(const Engine* pEngine) {

			if (!this->pDevice) {
				this->pDevice = reinterpret_cast<IDirect3DDevice9*>(pEngine->pHookArg);
			}

			pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) const { 
			UNREFERENCED_PARAMETER(pEngine);
			
			return; 
		}


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

			// triangle count is vertex count - 2
			this->pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, count - 2, data, sizeof(Vertex));
			delete[] data;

			return;
		}


		void Draw::drawString(void* pFont, const Vector2* pos, const char* text, rgb::Color color) const {
			Font* pDx9Font = reinterpret_cast<Font*>(pFont);

			if (!this->pDevice || !pDx9Font) return;

			const size_t size = strlen(text);

			for (size_t indexInString = 0; indexInString < size; indexInString++) {
				const Fontchar* pCurChar = nullptr;

				switch (text[indexInString]) {
				case '!':
					pCurChar = &pDx9Font->pCharset->exclamationPoint;
					break;
				case '"':
					pCurChar = &pDx9Font->pCharset->doubleQuotes;
					break;
				case '#':
					pCurChar = &pDx9Font->pCharset->poundSign;
					break;
				case '$':
					pCurChar = &pDx9Font->pCharset->dollarSign;
					break;
				case '%':
					pCurChar = &pDx9Font->pCharset->percentSign;
					break;
				case '&':
					pCurChar = &pDx9Font->pCharset->ampersand;
					break;
				case '\'':
					pCurChar = &pDx9Font->pCharset->singleQuote;
					break;
				case '(':
					pCurChar = &pDx9Font->pCharset->openParanthesis;
					break;
				case ')':
					pCurChar = &pDx9Font->pCharset->closeParanthesis;
					break;
				case '*':
					pCurChar = &pDx9Font->pCharset->asterisk;
					break;
				case '+':
					pCurChar = &pDx9Font->pCharset->plus;
					break;
				case ',':
					pCurChar = &pDx9Font->pCharset->comma;
					break;
				case '-':
					pCurChar = &pDx9Font->pCharset->dash;
					break;
				case '.':
					pCurChar = &pDx9Font->pCharset->period;
					break;
				case '/':
					pCurChar = &pDx9Font->pCharset->slash;
					break;
				case '0':
					pCurChar = &pDx9Font->pCharset->zero;
					break;
				case '1':
					pCurChar = &pDx9Font->pCharset->one;
					break;
				case '2':
					pCurChar = &pDx9Font->pCharset->two;
					break;
				case '3':
					pCurChar = &pDx9Font->pCharset->three;
					break;
				case '4':
					pCurChar = &pDx9Font->pCharset->four;
					break;
				case '5':
					pCurChar = &pDx9Font->pCharset->five;
					break;
				case '6':
					pCurChar = &pDx9Font->pCharset->six;
					break;
				case '7':
					pCurChar = &pDx9Font->pCharset->seven;
					break;
				case '8':
					pCurChar = &pDx9Font->pCharset->eight;
					break;
				case '9':
					pCurChar = &pDx9Font->pCharset->nine;
					break;
				case ':':
					pCurChar = &pDx9Font->pCharset->colon;
					break;
				case ';':
					pCurChar = &pDx9Font->pCharset->semicolon;
					break;
				case '<':
					pCurChar = &pDx9Font->pCharset->smaller;
					break;
				case '=':
					pCurChar = &pDx9Font->pCharset->equals;
					break;
				case '>':
					pCurChar = &pDx9Font->pCharset->greater;
					break;
				case '?':
					pCurChar = &pDx9Font->pCharset->questionmark;
					break;
				case '@':
					pCurChar = &pDx9Font->pCharset->atSign;
					break;
				case 'A':
					pCurChar = &pDx9Font->pCharset->upperA;
					break;
				case 'B':
					pCurChar = &pDx9Font->pCharset->upperB;
					break;
				case 'C':
					pCurChar = &pDx9Font->pCharset->upperC;
					break;
				case 'D':
					pCurChar = &pDx9Font->pCharset->upperD;
					break;
				case 'E':
					pCurChar = &pDx9Font->pCharset->upperE;
					break;
				case 'F':
					pCurChar = &pDx9Font->pCharset->upperF;
					break;
				case 'G':
					pCurChar = &pDx9Font->pCharset->upperG;
					break;
				case 'H':
					pCurChar = &pDx9Font->pCharset->upperH;
					break;
				case 'I':
					pCurChar = &pDx9Font->pCharset->upperI;
					break;
				case 'J':
					pCurChar = &pDx9Font->pCharset->upperJ;
					break;
				case 'K':
					pCurChar = &pDx9Font->pCharset->upperK;
					break;
				case 'L':
					pCurChar = &pDx9Font->pCharset->upperL;
					break;
				case 'M':
					pCurChar = &pDx9Font->pCharset->upperM;
					break;
				case 'N':
					pCurChar = &pDx9Font->pCharset->upperN;
					break;
				case 'O':
					pCurChar = &pDx9Font->pCharset->upperO;
					break;
				case 'P':
					pCurChar = &pDx9Font->pCharset->upperP;
					break;
				case 'Q':
					pCurChar = &pDx9Font->pCharset->upperQ;
					break;
				case 'R':
					pCurChar = &pDx9Font->pCharset->upperR;
					break;
				case 'S':
					pCurChar = &pDx9Font->pCharset->upperS;
					break;
				case 'T':
					pCurChar = &pDx9Font->pCharset->upperT;
					break;
				case 'U':
					pCurChar = &pDx9Font->pCharset->upperU;
					break;
				case 'V':
					pCurChar = &pDx9Font->pCharset->upperV;
					break;
				case 'W':
					pCurChar = &pDx9Font->pCharset->upperW;
					break;
				case 'X':
					pCurChar = &pDx9Font->pCharset->upperX;
					break;
				case 'Y':
					pCurChar = &pDx9Font->pCharset->upperY;
					break;
				case 'Z':
					pCurChar = &pDx9Font->pCharset->upperZ;
					break;
				case '[':
					pCurChar = &pDx9Font->pCharset->openBracket;
					break;
				case '\\':
					pCurChar = &pDx9Font->pCharset->backslash;
					break;
				case ']':
					pCurChar = &pDx9Font->pCharset->closeBracket;
					break;
				case '^':
					pCurChar = &pDx9Font->pCharset->circumflex;
					break;
				case '_':
					pCurChar = &pDx9Font->pCharset->underscore;
					break;
				case '`':
					pCurChar = &pDx9Font->pCharset->backtick;
					break;
				case 'a':
					pCurChar = &pDx9Font->pCharset->lowerA;
					break;
				case 'b':
					pCurChar = &pDx9Font->pCharset->lowerB;
					break;
				case 'c':
					pCurChar = &pDx9Font->pCharset->lowerC;
					break;
				case 'd':
					pCurChar = &pDx9Font->pCharset->lowerD;
					break;
				case 'e':
					pCurChar = &pDx9Font->pCharset->lowerE;
					break;
				case 'f':
					pCurChar = &pDx9Font->pCharset->lowerF;
					break;
				case 'g':
					pCurChar = &pDx9Font->pCharset->lowerG;
					break;
				case 'h':
					pCurChar = &pDx9Font->pCharset->lowerH;
					break;
				case 'i':
					pCurChar = &pDx9Font->pCharset->lowerI;
					break;
				case 'j':
					pCurChar = &pDx9Font->pCharset->lowerJ;
					break;
				case 'k':
					pCurChar = &pDx9Font->pCharset->lowerK;
					break;
				case 'l':
					pCurChar = &pDx9Font->pCharset->lowerL;
					break;
				case 'm':
					pCurChar = &pDx9Font->pCharset->lowerM;
					break;
				case 'n':
					pCurChar = &pDx9Font->pCharset->lowerN;
					break;
				case 'o':
					pCurChar = &pDx9Font->pCharset->lowerO;
					break;
				case 'p':
					pCurChar = &pDx9Font->pCharset->lowerP;
					break;
				case 'q':
					pCurChar = &pDx9Font->pCharset->lowerQ;
					break;
				case 'r':
					pCurChar = &pDx9Font->pCharset->lowerR;
					break;
				case 's':
					pCurChar = &pDx9Font->pCharset->lowerS;
					break;
				case 't':
					pCurChar = &pDx9Font->pCharset->lowerT;
					break;
				case 'u':
					pCurChar = &pDx9Font->pCharset->lowerU;
					break;
				case 'v':
					pCurChar = &pDx9Font->pCharset->lowerV;
					break;
				case 'w':
					pCurChar = &pDx9Font->pCharset->lowerW;
					break;
				case 'x':
					pCurChar = &pDx9Font->pCharset->lowerX;
					break;
				case 'y':
					pCurChar = &pDx9Font->pCharset->lowerY;
					break;
				case 'z':
					pCurChar = &pDx9Font->pCharset->lowerZ;
					break;
				case '{':
					pCurChar = &pDx9Font->pCharset->openBrace;
					break;
				case '|':
					pCurChar = &pDx9Font->pCharset->pipe;
					break;
				case '}':
					pCurChar = &pDx9Font->pCharset->closeBrace;
					break;
				case '~':
					pCurChar = &pDx9Font->pCharset->tilde;
					break;
				case '°':
					pCurChar = &pDx9Font->pCharset->degrees;
					break;
				}

				if (pCurChar && pCurChar->pixel) {
					// the index of the character in the charset array of the class is the same as the offset in the charset structure
					// bit of a hack
					const size_t indexInCharset = pCurChar - &pDx9Font->pCharset->exclamationPoint;
					// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
					const Vector2 curPos{ pos->x + (pDx9Font->pCharset->width + 1) * indexInString, pos->y - pDx9Font->pCharset->height };
					this->drawFontchar(pDx9Font, pCurChar, &curPos, indexInCharset, color);
				}

			}

			return;
		}


		HRESULT Draw::drawFontchar(Font* pDx9Font, const Fontchar* pChar, const Vector2* pos, size_t index, rgb::Color color) const {

			if (!this->pDevice || !pDx9Font) return E_FAIL;

			// vertex array is allocated at first use
			// array is not deleted during object lifetime for (meassurable) performance improvements
			if (!pDx9Font->charVertexArrays[index]) {
				pDx9Font->charVertexArrays[index] = new Vertex[pChar->pixelCount];
			}

			if (pDx9Font->charVertexArrays[index]) {

				for (unsigned int i = 0; i < pChar->pixelCount; i++) {
					pDx9Font->charVertexArrays[index][i].x = pos->x + pChar->pixel[i].x;
					pDx9Font->charVertexArrays[index][i].y = pos->y + pChar->pixel[i].y;
					pDx9Font->charVertexArrays[index][i].z = 1.f;
					pDx9Font->charVertexArrays[index][i].rhw = 1.f;
					pDx9Font->charVertexArrays[index][i].color = color;
				}

				return pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, pChar->pixelCount, pDx9Font->charVertexArrays[index], sizeof(Vertex));
			}

			return E_FAIL;
		}


		static BOOL CALLBACK getWindowCallback(HWND hWnd, LPARAM lParam) {
			DWORD processId = 0;
			GetWindowThreadProcessId(hWnd, &processId);

			if (!processId || GetCurrentProcessId() != processId) return TRUE;

			*reinterpret_cast<HWND*>(lParam) = hWnd;

			return FALSE;
		}

	}

}

