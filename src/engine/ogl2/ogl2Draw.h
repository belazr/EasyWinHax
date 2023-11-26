#pragma once
#include "..\Vertex.h"
#include "..\IDraw.h"
#define GL_GLEXT_PROTOTYPES
#include <gl\GL.h>

// Class for drawing with OpenGL 2.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace ogl2 {

		typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

		typedef void(APIENTRY* tglGenBuffers)(GLsizei n, GLuint* buffers);
		typedef void(APIENTRY* tglBindBuffer)(GLenum target, GLuint buffer);
		typedef void(APIENTRY* tglBufferData)(GLenum target, uintptr_t size, const GLvoid* data, GLenum usage);
		typedef void*(APIENTRY* tglMapBuffer)(GLenum target, GLenum access);
		typedef GLboolean (APIENTRY* tglUnmapBuffer)(GLenum target);
		typedef void(APIENTRY* tglDeleteBuffers)(GLsizei n, const GLuint* buffers);

		typedef struct BufferData {
			GLuint vertexBufferId;
			GLuint indexBufferId;
			Vertex* pLocalVertexBuffer;
			GLuint* pLocalIndexBuffer;
			UINT size;
			UINT curOffset;
		}BufferData;

		class Draw : public IDraw {
		private:
			tglGenBuffers _pglGenBuffers;
			tglBindBuffer _pglBindBuffer;
			tglBufferData _pglBufferData;
			tglMapBuffer _pglMapBuffer;
			tglUnmapBuffer _pglUnmapBuffer;
			tglDeleteBuffers _pglDeleteBuffers;

			GLint _width;
			GLint _height;

			BufferData _triangleListBufferData;

			GLuint _lastIndexBufferId;
			GLuint _lastVertexBufferId;

			bool _isInit;

		public:
			Draw();

			~Draw();

			// Initializes drawing within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void beginDraw(Engine* pEngine) override;

			// Ends drawing within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			void endDraw(const Engine* pEngine) override;

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. For there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in]
			// Color of the triangle list.
			void drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) override;

			// Draws text to the screen. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pFont:
			// Pointer to an ogl2::Font object.
			//
			// [in] origin:
			// Coordinates of the bottom left corner of the first character of the text.
			// 
			// [in] text:
			// Text to be drawn. See length limitations implementations.
			//
			// [in] color:
			// Color of the text.
			void drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) override;

		private:
			bool getProcAddresses();
			bool createBufferData(BufferData* pBufferData, UINT size) const;
			bool createBuffer(GLenum target, GLenum binding, UINT size, GLuint* pId) const;
			bool resizeBuffers(BufferData* pBufferData, UINT newSize) const;
		};

	}

}

