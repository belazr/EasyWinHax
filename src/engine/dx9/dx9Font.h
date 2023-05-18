#pragma once
#include "charsets\dx9Charsets.h"
#include <d3d9.h>

// Class to specify a text font for DirectX 9 EndScenes hook drawing without the need to include the deprecated DirectX 9 SDK.
// Uses the charset structure which defines width and height of a character and the pixel coordinates of every character.
// Characters are drawn monospaced.

namespace hax {

	namespace dx9 {

		typedef struct Vertex {
			float x, y, z, rhw;
			D3DCOLOR color;
		}Vertex;

		class Font {
		public:
			const Charset* pCharset;
			Vertex* charVertexArrays[CHAR_COUNT];

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