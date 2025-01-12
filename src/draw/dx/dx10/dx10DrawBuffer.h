#pragma once
#include "..\..\AbstractDrawBuffer.h"
#include <d3d10.h>

namespace hax {

	namespace draw {

		namespace dx10 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:

			public:
				DrawBuffer();

				~DrawBuffer();
			};

		}

	}

}