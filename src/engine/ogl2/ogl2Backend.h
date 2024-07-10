#pragma once
#include "..\Vertex.h"
#include "..\IBackend.h"
#include <stdint.h>
#include <gl\GL.h>

// Class for drawing with OpenGL 2.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace ogl2 {
		typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

		class Backend : public IBackend {
		private:
			typedef void(APIENTRY* tGlGenBuffers)(GLsizei n, GLuint* buffers);
			typedef void(APIENTRY* tGlBindBuffer)(GLenum target, GLuint buffer);
			typedef void(APIENTRY* tGlBufferData)(GLenum target, size_t size, const GLvoid* data, GLenum usage);
			typedef void* (APIENTRY* tGlMapBuffer)(GLenum target, GLenum access);
			typedef GLboolean(APIENTRY* tGlUnmapBuffer)(GLenum target);
			typedef void(APIENTRY* tGlDeleteBuffers)(GLsizei n, const GLuint* buffers);

			typedef struct Functions {
				tGlGenBuffers pGlGenBuffers;
				tGlBindBuffer pGlBindBuffer;
				tGlBufferData pGlBufferData;
				tGlMapBuffer pGlMapBuffer;
				tGlUnmapBuffer pGlUnmapBuffer;
				tGlDeleteBuffers pGlDeleteBuffers;
			}Functions;

			typedef struct BufferData {
				GLuint vertexBufferId;
				GLuint indexBufferId;
				Vertex* pLocalVertexBuffer;
				uint32_t* pLocalIndexBuffer;
				uint32_t vertexBufferSize;
				uint32_t indexBufferSize;
				uint32_t curOffset;
			}BufferData;

			union {
				Functions _f;
				void* _fPtrs[sizeof(Functions) / sizeof(void*)];
			};

			GLint _viewport[4];

			BufferData _triangleListBufferData;
			BufferData _pointListBufferData;

			bool _isInit;
			bool _isBegin;

		public:
			Backend();

			~Backend();

			// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			virtual void beginFrame(Engine* pEngine) override;

			// Ends the current frame within a hook. Should be called by an Engine object.
			// 
			// [in] pEngine:
			// Pointer to the Engine object responsible for drawing within the hook.
			virtual void endFrame(const Engine* pEngine) override;

			// Gets the resolution of the current frame. Should be called by an Engine object.
			//
			// Parameters:
			// 
			// [out] frameWidth:
			// Pointer that receives the current frame width in pixel.
			//
			// [out] frameHeight:
			// Pointer that receives the current frame height in pixel.
			virtual void getFrameResolution(float* frameWidth, float* frameHeight) override;

			// Draws a filled triangle list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] corners:
			// Screen coordinates of the corners of the triangles in the list.
			// The three corners of the first triangle have to be in clockwise order. From there on the orientation of the triangles has to alternate.
			// 
			// [in] count:
			// Count of the corners of the triangles in the list. Has to be divisble by three.
			// 
			// [in] color:
			// Color of each triangle.
			void drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) override;

			// Draws a point list. Should be called by an Engine object.
			// 
			// Parameters:
			// 
			// [in] coordinate:
			// Screen coordinates of the points in the list.
			// 
			// [in] count:
			// Count of the points in the list.
			// 
			// [in] color:
			// Color of each point.
			//
			// [in] offest:
			// Offset by which each point is drawn.
			virtual void drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) override;

		private:
			bool initialize();
			bool getProcAddresses();
			bool createBufferData(BufferData* pBufferData, uint32_t vertexCount) const;
			void destroyBufferData(BufferData* pBufferData) const;
			bool createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const;
			bool mapBufferData(BufferData* pBufferData) const;
			void copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset = { 0.f, 0.f }) const;
			bool resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const;
			void drawBufferData(BufferData* pBufferData, GLenum mode) const;
		};

	}

}

