#include "dxFont.h"

namespace hax {

	namespace dx {

		template <typename V>
		Font<V>::Font(const Charset* pChrSet) : pCharset{ pChrSet }, charVerticesArrays{} {
			
			for (unsigned int i = 0; i < CharIndex::MAX_CHAR; i++) {
				charVerticesArrays[i] = reinterpret_cast<V*>(new BYTE[pCharset->chars[i].pixelCount * sizeof(V)]);
			}
			
		}

		
		template <typename V>
		Font<V>::~Font() {

			for (unsigned int i = 0; i < CharIndex::MAX_CHAR; i++) {

				if (this->charVerticesArrays[i]) {
					delete[] this->charVerticesArrays[i];
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

		// explicit instantiation to get a compilation in a static library
		template class Font<dx9::Vertex>;
		template class Font<dx11::Vertex>;

	}

}