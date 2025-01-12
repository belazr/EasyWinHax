#pragma once
#include "dx10DrawBuffer.h"
#include "..\..\IBackend.h"
#include "..\..\Vertex.h"
#include <d3d10.h>

// Class for drawing within a DirectX 10 Present hook.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace dx10 {

			class Backend : public IBackend {
			private:

			public:
				Backend();

				~Backend();
			};

		}

	}

}