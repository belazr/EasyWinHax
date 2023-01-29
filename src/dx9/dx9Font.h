#pragma once
#include "dx9Charsets.h"
#include "dx9Draw.h"
#include <d3d9.h>

// Class to draw text within a DirectX 9 EndScenes hook without the need to include the deprecated DirectX 9 SDK.
// Uses the charset structure which defines width and height of a character and the pixel coordinates of every character.
// Characters are drawn monospaced.

namespace dx9 {
	
	class Font {
	private:
		static const size_t CHAR_COUNT = (sizeof(Charset) - 2 * sizeof(float)) / sizeof(Fontchar);
		const Charset* _pCharset;
		D3DCOLOR _color;
		Vertex* _charVertexArrays[CHAR_COUNT];

	public:
		// Allocate memory for the character vertex arrays and initializes members.
		//
		// Parameters:
		//
		// [in] pCharset:
		// Pointer to a charset structure. Different structures can be used for different font sizes.
		// 
		// [in] color:
		// Color of drawn text. 
		Font(const Charset* pCharset, D3DCOLOR color);
		~Font();

		// Draws a text to the screen.
		//
		// Parameters:
		//
		// [in] origin:
		// Coordinates of the bottom left corner of the first character of the text.
		// 
		// [in] text:
		// Text to be drawn. 
		void drawPrimitiveString(const Vector2* origin, const char* text);

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

		void setCharset(const Charset* pCharset);
		void setColor(D3DCOLOR color);

	private:
		HRESULT drawPrimitiveFontchar(const Fontchar* pChar, const Vector2* pos, size_t index);
	};

 }