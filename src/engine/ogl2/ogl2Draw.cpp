#include "ogl2Draw.h"
#include "ogl2Font.h"
#include "..\Engine.h"
#include "..\..\Bench.h"
#include <Windows.h>

namespace hax {

	namespace ogl2 {

		static Bench bench{ "MakeCurrentContext", 200 };

		Draw::Draw() : _hGameContext{}, _hHookContext{}, _width{}, _height{},
			_pglGenBuffers{}, _pglBindBuffer{}, _pglBufferData{}, _pglMapBuffer{},
			_pglUnmapBuffer{}, _pglDeleteBuffers {}, _triangleListBufferData{}, 
			_lastVertexBufferId{}, _lastIndexBufferId{}, _isInit {} {}


		constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
		constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;

		Draw::~Draw() {

			if (this->_triangleListBufferData.indexBufferId) {
				this->_pglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->_pglDeleteBuffers(1, &this->_triangleListBufferData.indexBufferId);
			}
			
			if (this->_triangleListBufferData.vertexBufferId) {
				this->_pglUnmapBuffer(GL_ARRAY_BUFFER);
				this->_pglDeleteBuffers(1, &this->_triangleListBufferData.vertexBufferId);
			}

			if (this->_hHookContext) {
				wglDeleteContext(this->_hHookContext);
			}

		}

		constexpr GLenum GL_ARRAY_BUFFER_BINDING = 0x8894;
		constexpr GLenum GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8895;
		constexpr GLenum GL_WRITE_ONLY = 0x88B9;

		void Draw::beginDraw(Engine* pEngine) {
			const HDC hDc = reinterpret_cast<HDC>(pEngine->pHookArg);

			if (!this->_isInit) {

				if (!this->getProcAddresses()) return;

				this->_hGameContext = wglGetCurrentContext();
				this->_hHookContext = wglCreateContext(hDc);
				
				if (!this->_hGameContext || !this->_hHookContext) return;

				//if (!wglMakeCurrent(hDc, this->_hHookContext)) return;

				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&this->_lastVertexBufferId));
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&this->_lastIndexBufferId));

				constexpr GLsizei INITIAL_TRIANGLE_LIST_BUFFER_SIZE = sizeof(Vertex) * 4;

				if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;
				
				this->_isInit = true;
			}
			
			//wglMakeCurrent(hDc, this->_hHookContext);

			GLint viewport[4]{};
			glGetIntegerv(GL_VIEWPORT, viewport);

			if (this->_width == viewport[2] && this->_height == viewport[3]) return;

			this->_width = viewport[2];
			this->_height = viewport[3];
			
			pEngine->setWindowSize(this->_width, this->_height);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, static_cast<double>(this->_width), static_cast<double>(this->_height), 0, -1.f, 1.f);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			this->_triangleListBufferData.pLocalIndexBuffer = reinterpret_cast<GLuint*>(this->_pglMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
			this->_triangleListBufferData.pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_pglMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);

			this->_pglUnmapBuffer(GL_ARRAY_BUFFER);
			this->_pglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

			// third argument describes the offset in the Vertext struct, not the actuall address of the buffer
			glVertexPointer(2, GL_FLOAT, sizeof(Vertex), nullptr);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));
			glDrawElements(GL_TRIANGLES, this->_triangleListBufferData.curOffset, GL_UNSIGNED_INT, nullptr);
			
			this->_triangleListBufferData.curOffset = 0;

			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			this->_pglBindBuffer(GL_ARRAY_BUFFER, this->_lastVertexBufferId);
			this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_lastIndexBufferId);

			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();

			return;
		}

		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {
			
			if (!this->_isInit || count % 3) return;
			
			const UINT sizeNeeded = (this->_triangleListBufferData.curOffset + count) * sizeof(Vertex);

			if (sizeNeeded > this->_triangleListBufferData.size) {

				if (!this->resizeBuffers(&this->_triangleListBufferData, sizeNeeded * 2)) return;

			}

			for (UINT i = 0; i < count; i++) {
				const UINT curIndex = this->_triangleListBufferData.curOffset + i;

				Vertex curVertex{ { corners[i].x, corners[i].y }, color };
				memcpy(&(this->_triangleListBufferData.pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

				this->_triangleListBufferData.pLocalIndexBuffer[curIndex] = curIndex;
			}

			this->_triangleListBufferData.curOffset += count;

			return;
        }


		void Draw::drawString(const void* pFont, const Vector2* pos, const char* text, rgb::Color color) {
			
			if (!this->_isInit) return;

			const Font* const pOgl2Font = reinterpret_cast<const Font*>(pFont);
			GLsizei strLen = static_cast<GLsizei>(strlen(text));
			
			if (!pOgl2Font || strLen > 100) return;

			glColor3ub(UCHAR_R(color), UCHAR_G(color), UCHAR_B(color));
			glRasterPos2f(pos->x, pos->y);
			glPushAttrib(GL_LIST_BIT);
			glListBase(pOgl2Font->displayLists - 32);
			glCallLists(strLen, GL_UNSIGNED_BYTE, text);
			glPopAttrib();

            return;
		}


		bool Draw::getProcAddresses()
		{
			this->_pglGenBuffers = reinterpret_cast<tglGenBuffers>(wglGetProcAddress("glGenBuffers"));
			this->_pglBindBuffer = reinterpret_cast<tglBindBuffer>(wglGetProcAddress("glBindBuffer"));
			this->_pglBufferData = reinterpret_cast<tglBufferData>(wglGetProcAddress("glBufferData"));
			this->_pglMapBuffer = reinterpret_cast<tglMapBuffer>(wglGetProcAddress("glMapBuffer"));
			this->_pglUnmapBuffer = reinterpret_cast<tglUnmapBuffer>(wglGetProcAddress("glUnmapBuffer"));
			this->_pglDeleteBuffers = reinterpret_cast<tglDeleteBuffers>(wglGetProcAddress("glDeleteBuffers"));

			if (
				!this->_pglGenBuffers || !this->_pglBindBuffer || !this->_pglBufferData ||
				!this->_pglMapBuffer || !this->_pglUnmapBuffer || !this->_pglDeleteBuffers
			) {
				return false;
			}

			return true;
		}


		bool Draw::createBufferData(BufferData* pBufferData, UINT size) const {
			constexpr GLenum GL_DYNAMIC_DRAW = 0x88E8;
			
			this->_pglGenBuffers(1, &pBufferData->indexBufferId);

			if (!pBufferData->indexBufferId) return false;

			this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBufferData->indexBufferId);
			this->_pglBufferData(GL_ELEMENT_ARRAY_BUFFER, size / sizeof(Vertex) * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
			
			this->_pglGenBuffers(1, &pBufferData->vertexBufferId);

			if (!pBufferData->vertexBufferId) return false;

			this->_pglBindBuffer(GL_ARRAY_BUFFER, pBufferData->vertexBufferId);
			this->_pglBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
			
			pBufferData->size = size;

			return true;
		}


		bool Draw::resizeBuffers(BufferData* pBufferData, UINT newSize) const {
			const UINT bytesUsedVertex = pBufferData->curOffset * sizeof(Vertex);

			if (newSize < bytesUsedVertex) return false;

			const BufferData oldBufferData = *pBufferData;

			if (!this->createBufferData(pBufferData, newSize)) return false;

			pBufferData->pLocalIndexBuffer = reinterpret_cast<GLuint*>(this->_pglMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

			if (oldBufferData.pLocalIndexBuffer) {
				memcpy(pBufferData->pLocalIndexBuffer, oldBufferData.pLocalIndexBuffer, bytesUsedVertex / sizeof(Vertex) * sizeof(GLuint));

				this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oldBufferData.indexBufferId);
				this->_pglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->_pglDeleteBuffers(1, &oldBufferData.indexBufferId);
				this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBufferData->indexBufferId);
			}

			pBufferData->pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_pglMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			if (oldBufferData.pLocalVertexBuffer) {
				memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, bytesUsedVertex);

				this->_pglBindBuffer(GL_ARRAY_BUFFER, oldBufferData.vertexBufferId);
				this->_pglUnmapBuffer(GL_ARRAY_BUFFER);
				this->_pglDeleteBuffers(1, &oldBufferData.vertexBufferId);
				this->_pglBindBuffer(GL_ARRAY_BUFFER, pBufferData->vertexBufferId);
			}
			
			return true;
		}

	}

}