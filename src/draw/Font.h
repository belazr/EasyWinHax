#pragma once
#include "Color.h"
#include "IBufferBackend.h"

namespace hax {

	namespace draw {

		struct Font {
			const Color* pTexture;
			const uint32_t width;
			const uint32_t height;
			const uint32_t charWidth;
			TextureId textureId;
		};

		namespace fonts {
			
			extern Font inconsolata;
		
		}

	}

}