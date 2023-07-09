#pragma once
#include "charsets\dxCharsets.h"
#include "..\dx9\dx9Vertex.h"
#include "..\dx11\dx11Vertex.h"
#include <d3d9.h>

// Class for fonts for text drawing within a DirectX hook without the need to include the deprecated DirectX 9 SDK.
// Uses the Charset structure which defines width and height of a character and the pixel coordinates of every character.
// Characters are drawn monospaced.

namespace hax {

	namespace dx {

		template <typename V>
		class Font {
		public:
			const Charset* const pCharset;
			// Array for vertices of the charset characters.
			V* charVerticesArrays[CharIndex::MAX_CHAR];

			// Allocate memory for the character vertex arrays and initializes members.
			//
			// Parameters:
			//
			// [in] pCharset:
			// Pointer to a charset structure. Different structures can be used for different font sizes.
			// 
			// [in] color:
			// Color of drawn text. 
			Font(const Charset* pChrSet);
			~Font();

			// Gets the width of a character in pixels.
			// 
			// Return:
			// The width of a character in pixels.
			float getCharWidth() const;

			// Gets the height of a character in pixels.
			// 
			// Return:
			// The height of a character in pixels.
			float getCharHeight() const;
		};

	}

}