#pragma once
#include "ogl2Defs.h"
#include "..\AbstractDrawBuffer.h"

namespace hax {

	namespace draw {

		namespace ogl2 {

			class DrawBuffer : public AbstractDrawBuffer {
			private:
				Functions _f;
				GLuint _vertexBufferId;
				GLuint _indexBufferId;
				GLenum _mode;

			public:
				DrawBuffer(Functions f, GLenum mode);
				~DrawBuffer();

				bool create(uint32_t vertexCount) override;
				void destroy() override;
				bool map() override;
				void draw() override;

			private:
				bool createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const;
			};

		}

	}

}