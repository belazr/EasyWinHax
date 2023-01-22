#include "dx9Font.h"

namespace dx9 {
	
	Font::Font(const Charset* pCharset, D3DCOLOR color) : _pCharset(pCharset), _color(color), _charVertexArrays{} {}

	
	Font::~Font() {
		
		// character vertex arrays are only deleted at object destrucion for (meassurable) performance reasons
		for (int i = 0; i < CHAR_COUNT; i++) {
			
			if (this->_charVertexArrays[i]) {
				delete[] this->_charVertexArrays[i];
			}

		}

	}


	void Font::drawPrimitiveString(const Vector2* origin, const char* text) {
		size_t size = strlen(text);

		for (size_t indexInString = 0; indexInString < size; indexInString++) {
			const Fontchar* pCurChar = nullptr;

			switch (text[indexInString]) {
			case '!':
				pCurChar = &this->_pCharset->exclamationPoint;
				break;
			case '"':
				pCurChar = &this->_pCharset->doubleQuotes;
				break;
			case '#':
				pCurChar = &this->_pCharset->poundSign;
				break;
			case '$':
				pCurChar = &this->_pCharset->dollarSign;
				break;
			case '%':
				pCurChar = &this->_pCharset->percentSign;
				break;
			case '&':
				pCurChar = &this->_pCharset->ampersand;
				break;
			case '\'':
				pCurChar = &this->_pCharset->singleQuote;
				break;
			case '(':
				pCurChar = &this->_pCharset->openParanthesis;
				break;
			case ')':
				pCurChar = &this->_pCharset->closeParanthesis;
				break;
			case '*':
				pCurChar = &this->_pCharset->asterisk;
				break;
			case '+':
				pCurChar = &this->_pCharset->plus;
				break;
			case ',':
				pCurChar = &this->_pCharset->comma;
				break;
			case '-':
				pCurChar = &this->_pCharset->dash;
				break;
			case '.':
				pCurChar = &this->_pCharset->period;
				break;
			case '/':
				pCurChar = &this->_pCharset->slash;
				break;
			case '0':
				pCurChar = &this->_pCharset->zero;
				break;
			case '1':
				pCurChar = &this->_pCharset->one;
				break;
			case '2':
				pCurChar = &this->_pCharset->two;
				break;
			case '3':
				pCurChar = &this->_pCharset->three;
				break;
			case '4':
				pCurChar = &this->_pCharset->four;
				break;
			case '5':
				pCurChar = &this->_pCharset->five;
				break;
			case '6':
				pCurChar = &this->_pCharset->six;
				break;
			case '7':
				pCurChar = &this->_pCharset->seven;
				break;
			case '8':
				pCurChar = &this->_pCharset->eight;
				break;
			case '9':
				pCurChar = &this->_pCharset->nine;
				break;
			case ':':
				pCurChar = &this->_pCharset->colon;
				break;
			case ';':
				pCurChar = &this->_pCharset->semicolon;
				break;
			case '<':
				pCurChar = &this->_pCharset->smaller;
				break;
			case '=':
				pCurChar = &this->_pCharset->equals;
				break;
			case '>':
				pCurChar = &this->_pCharset->greater;
				break;
			case '?':
				pCurChar = &this->_pCharset->questionmark;
				break;
			case '@':
				pCurChar = &this->_pCharset->atSign;
				break;
			case 'A':
				pCurChar = &this->_pCharset->upperA;
				break;
			case 'B':
				pCurChar = &this->_pCharset->upperB;
				break;
			case 'C':
				pCurChar = &this->_pCharset->upperC;
				break;
			case 'D':
				pCurChar = &this->_pCharset->upperD;
				break;
			case 'E':
				pCurChar = &this->_pCharset->upperE;
				break;
			case 'F':
				pCurChar = &this->_pCharset->upperF;
				break;
			case 'G':
				pCurChar = &this->_pCharset->upperG;
				break;
			case 'H':
				pCurChar = &this->_pCharset->upperH;
				break;
			case 'I':
				pCurChar = &this->_pCharset->upperI;
				break;
			case 'J':
				pCurChar = &this->_pCharset->upperJ;
				break;
			case 'K':
				pCurChar = &this->_pCharset->upperK;
				break;
			case 'L':
				pCurChar = &this->_pCharset->upperL;
				break;
			case 'M':
				pCurChar = &this->_pCharset->upperM;
				break;
			case 'N':
				pCurChar = &this->_pCharset->upperN;
				break;
			case 'O':
				pCurChar = &this->_pCharset->upperO;
				break;
			case 'P':
				pCurChar = &this->_pCharset->upperP;
				break;
			case 'Q':
				pCurChar = &this->_pCharset->upperQ;
				break;
			case 'R':
				pCurChar = &this->_pCharset->upperR;
				break;
			case 'S':
				pCurChar = &this->_pCharset->upperS;
				break;
			case 'T':
				pCurChar = &this->_pCharset->upperT;
				break;
			case 'U':
				pCurChar = &this->_pCharset->upperU;
				break;
			case 'V':
				pCurChar = &this->_pCharset->upperV;
				break;
			case 'W':
				pCurChar = &this->_pCharset->upperW;
				break;
			case 'X':
				pCurChar = &this->_pCharset->upperX;
				break;
			case 'Y':
				pCurChar = &this->_pCharset->upperY;
				break;
			case 'Z':
				pCurChar = &this->_pCharset->upperZ;
				break;
			case '[':
				pCurChar = &this->_pCharset->openBracket;
				break;
			case '\\':
				pCurChar = &this->_pCharset->backslash;
				break;
			case ']':
				pCurChar = &this->_pCharset->closeBracket;
				break;
			case '^':
				pCurChar = &this->_pCharset->circumflex;
				break;
			case '_':
				pCurChar = &this->_pCharset->underscore;
				break;
			case '`':
				pCurChar = &this->_pCharset->backtick;
				break;
			case 'a':
				pCurChar = &this->_pCharset->lowerA;
				break;
			case 'b':
				pCurChar = &this->_pCharset->lowerB;
				break;
			case 'c':
				pCurChar = &this->_pCharset->lowerC;
				break;
			case 'd':
				pCurChar = &this->_pCharset->lowerD;
				break;
			case 'e':
				pCurChar = &this->_pCharset->lowerE;
				break;
			case 'f':
				pCurChar = &this->_pCharset->lowerF;
				break;
			case 'g':
				pCurChar = &this->_pCharset->lowerG;
				break;
			case 'h':
				pCurChar = &this->_pCharset->lowerH;
				break;
			case 'i':
				pCurChar = &this->_pCharset->lowerI;
				break;
			case 'j':
				pCurChar = &this->_pCharset->lowerJ;
				break;
			case 'k':
				pCurChar = &this->_pCharset->lowerK;
				break;
			case 'l':
				pCurChar = &this->_pCharset->lowerL;
				break;
			case 'm':
				pCurChar = &this->_pCharset->lowerM;
				break;
			case 'n':
				pCurChar = &this->_pCharset->lowerN;
				break;
			case 'o':
				pCurChar = &this->_pCharset->lowerO;
				break;
			case 'p':
				pCurChar = &this->_pCharset->lowerP;
				break;
			case 'q':
				pCurChar = &this->_pCharset->lowerQ;
				break;
			case 'r':
				pCurChar = &this->_pCharset->lowerR;
				break;
			case 's':
				pCurChar = &this->_pCharset->lowerS;
				break;
			case 't':
				pCurChar = &this->_pCharset->lowerT;
				break;
			case 'u':
				pCurChar = &this->_pCharset->lowerU;
				break;
			case 'v':
				pCurChar = &this->_pCharset->lowerV;
				break;
			case 'w':
				pCurChar = &this->_pCharset->lowerW;
				break;
			case 'x':
				pCurChar = &this->_pCharset->lowerX;
				break;
			case 'y':
				pCurChar = &this->_pCharset->lowerY;
				break;
			case 'z':
				pCurChar = &this->_pCharset->lowerZ;
				break;
			case '{':
				pCurChar = &this->_pCharset->openBrace;
				break;
			case '|':
				pCurChar = &this->_pCharset->pipe;
				break;
			case '}':
				pCurChar = &this->_pCharset->closeBrace;
				break;
			case '~':
				pCurChar = &this->_pCharset->tilde;
				break;
			case '°':
				pCurChar = &this->_pCharset->degrees;
				break;
			}

			if (pCurChar && pCurChar->pixel) {
				// the index of the character in the charset array of the class is the same as the offset in the charset structure
				// bit of a hack
				const size_t indexInCharset = pCurChar - &this->_pCharset->exclamationPoint;
				// current char x coordinate is offset by width of previously drawn chars plus one pixel spacing per char
				const Vector2 pos{ origin->x + (this->_pCharset->width + 1) * indexInString, origin->y - this->_pCharset->height };
				drawPrimitiveFontchar(pCurChar, &pos, indexInCharset);
			}

		}

		return;
	}


	void Font::setCharset(const Charset* pCharset) {
		this->_pCharset = pCharset;
		
		return;
	}


	void Font::setColor(D3DCOLOR color) {
		this->_color = color;

		return;
	}

	
	float Font::getCharWidth() const {
		
		return this->_pCharset->width;
	}

	
	float Font::getCharHeight() const {
		
		return this->_pCharset->height;
	}


	HRESULT Font::drawPrimitiveFontchar(const Fontchar* pChar, const Vector2* pos, size_t index) {
		
		// vertex array is allocated at first use
		// array is not deleted during object lifetime for (meassurable) performance reasons
		if (!this->_charVertexArrays[index]) {
			this->_charVertexArrays[index] = new Vertex[pChar->pixelCount]{};
		}

		if (this->_charVertexArrays[index]) {
			
			for (unsigned int i = 0; i < pChar->pixelCount; i++) {
				this->_charVertexArrays[index][i].x = pos->x + pChar->pixel[i].x;
				this->_charVertexArrays[index][i].y = pos->y + pChar->pixel[i].y;
				this->_charVertexArrays[index][i].z = 1.0f;
				this->_charVertexArrays[index][i].rhw = 1.0f;
				this->_charVertexArrays[index][i].color = this->_color;
			}

			return pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, pChar->pixelCount, this->_charVertexArrays[index], sizeof(Vertex));
		}

		return E_FAIL;
	}

}