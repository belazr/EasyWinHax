#pragma once
#include "ogl2Defs.h"
#include "..\Vertex.h"
#include "..\IBackend.h"

// Class for drawing with OpenGL 2.
// All methods are intended to be called by an Engine object and not for direct calls.

namespace hax {

	namespace draw {

		namespace ogl2 {
			typedef BOOL(APIENTRY* twglSwapBuffers)(HDC hDc);

			class Backend : public IBackend {
			private:
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

			public:
				Backend();
				~Backend();

				// Initializes backend and starts a frame within a hook. Should be called by an Engine object.
				//
				// Parameters:
				// 
				// [in] pArg1:
				// Pass nothing.
				//
				// [in] pArg2:
				// Pass nothing
				//
				// [in] pArg3:
				// Pass nothing
				virtual void setHookArguments(void* pArg1 = nullptr, const void* pArg2 = nullptr, void* pArg3 = nullptr) override;

				// Initializes the backend. Should be called by an Engine object until success.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool initialize() override;

				// Starts a frame within a hook. Should be called by an Engine object every frame at the begin of the hook.
				// 
				// Return:
				// True on success, false on failure.
				virtual bool beginFrame() override;

				// Ends the current frame within a hook. Should be called by an Engine object every frame at the end of the hook.
				virtual void endFrame() override;

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

}