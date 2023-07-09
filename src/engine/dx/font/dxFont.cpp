#include "dxFont.h"

namespace hax {

	namespace dx {

		template <typename V>
		Font<V>::Font(const Charset* pChrSet) : pCharset{pChrSet}, charVertexArrays{} {}

		
		template <typename V>
		Font<V>::~Font() {

			// character vertex arrays are only deleted at object destrucion for (meassurable) performance reasons
			for (int i = 0; i < CHAR_COUNT; i++) {

				if (this->charVertexArrays[i]) {
					delete[] this->charVertexArrays[i];
				}

			}

		}


		template <typename V>
		float Font<V>::getCharWidth() const {

			return this->pCharset->width;
		}


		template <typename V>
		float Font<V>::getCharHeight() const {

			return this->pCharset->height;
		}

		// Explicit instantiation to get a compilation in a static library
		template class Font<dx9::Vertex>;
		template class Font<dx11::Vertex>;

	}

}