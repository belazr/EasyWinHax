#include "dx9Font.h"

namespace hax {

	namespace dx9 {

		Font::Font(const Charset* pChrSet) : pCharset(pChrSet), charVertexArrays{} {}


		Font::~Font() {

			// character vertex arrays are only deleted at object destrucion for (meassurable) performance reasons
			for (int i = 0; i < CHAR_COUNT; i++) {

				if (this->charVertexArrays[i]) {
					delete[] this->charVertexArrays[i];
				}

			}

		}


		float Font::getCharWidth() const {

			return this->pCharset->width;
		}


		float Font::getCharHeight() const {

			return this->pCharset->height;
		}

	}

}