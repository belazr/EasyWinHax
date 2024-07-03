#include "ogl2Draw.h"
#include "..\font\Font.h"
#include "..\Engine.h"
#include <Windows.h>

namespace hax {

	namespace ogl2 {

		constexpr static GLenum GL_ARRAY_BUFFER = 0x8892;
		constexpr static GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;
		constexpr static GLenum GL_ARRAY_BUFFER_BINDING = 0x8894;
		constexpr static GLenum GL_ELEMENT_ARRAY_BUFFER_BINDING = 0x8895;
		constexpr static GLenum GL_DYNAMIC_DRAW = 0x88E8;
		constexpr static GLenum GL_WRITE_ONLY = 0x88B9;

		Draw::Draw() : _f{}, _width{}, _height{},
			_triangleListBufferData{ UINT_MAX, UINT_MAX }, _pointListBufferData{ UINT_MAX, UINT_MAX },
			_isInit {} {}


		Draw::~Draw() {
			this->destroyBufferData(&this->_pointListBufferData);
			this->destroyBufferData(&this->_triangleListBufferData);
		}


		void Draw::beginDraw(Engine* pEngine) {

			if (!this->_isInit) {				
				this->_isInit = this->initialize();
			}

			GLint viewport[4]{};
			glGetIntegerv(GL_VIEWPORT, viewport);

			if (this->_width != viewport[2] || this->_height != viewport[3]) {
				this->_width = viewport[2];
				this->_height = viewport[3];

				pEngine->setWindowSize(this->_width, this->_height);
			}

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, static_cast<double>(this->_width), static_cast<double>(this->_height), 0, -1.f, 1.f);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			if (!this->mapBufferData(&this->_triangleListBufferData)) return;
			
			if (!this->mapBufferData(&this->_pointListBufferData)) return;

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {
			UNREFERENCED_PARAMETER(pEngine);

			if (!this->_isInit) return;
			
			this->drawBufferData(&this->_pointListBufferData, GL_POINTS);
			this->drawBufferData(&this->_triangleListBufferData, GL_TRIANGLES);
			
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();

			return;
		}

		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {
			
			if (!this->_isInit || count % 3) return;
			
			this->copyToBufferData(&this->_triangleListBufferData, corners, count, color);

			return;
        }


		void Draw::drawString(const font::Font* pFont, const Vector2* pos, const char* text, rgb::Color color) {

			if (!this->_isInit || !pFont) return;

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;

				const font::CharIndex index = font::charToCharIndex(c);
				const font::Char* pCurChar = &pFont->chars[index];

				if (pCurChar) {
					// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
					const Vector2 curPos{ pos->x + (pFont->width + 2.f) * i, pos->y - pFont->height };
					this->copyToBufferData(&this->_pointListBufferData, pCurChar->body.coordinates, pCurChar->body.count, color, curPos);
					this->copyToBufferData(&this->_pointListBufferData, pCurChar->outline.coordinates, pCurChar->outline.count, rgb::black, curPos);
				}

			}

			return;
		}


		bool Draw::initialize() {
			
			if (!this->getProcAddresses()) return false;

			this->destroyBufferData(&this->_triangleListBufferData);

			constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

			if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return false;

			this->destroyBufferData(&this->_pointListBufferData);

			constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 1000u;

			if (!this->createBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return false;
			
			return true;
		}


		#define ASSIGN_PROC_ADDRESS(f) this->_f.pGl##f = reinterpret_cast<tGl##f>(wglGetProcAddress("gl"#f))

		bool Draw::getProcAddresses() {
			ASSIGN_PROC_ADDRESS(GenBuffers);
			ASSIGN_PROC_ADDRESS(BindBuffer);
			ASSIGN_PROC_ADDRESS(BufferData);
			ASSIGN_PROC_ADDRESS(MapBuffer);
			ASSIGN_PROC_ADDRESS(UnmapBuffer);
			ASSIGN_PROC_ADDRESS(DeleteBuffers);

			for (size_t i = 0u; i < _countof(this->_fPtrs); i++) {

				if (!(this->_fPtrs[i])) return false;

			}

			return true;
		}

		#undef ASSIGN_PROC_ADDRESS


		bool Draw::createBufferData(BufferData* pBufferData, uint32_t vertexCount) const {
			RtlSecureZeroMemory(pBufferData, sizeof(BufferData));
			
			const uint32_t vertexBufferSize = vertexCount * sizeof(Vertex);
			
			if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBufferSize, &pBufferData->vertexBufferId)) return false;

			pBufferData->vertexBufferSize = vertexBufferSize;

			const uint32_t indexBufferSize = vertexCount * sizeof(GLuint);

			if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBufferSize, &pBufferData->indexBufferId)) return false;
			
			pBufferData->indexBufferSize = indexBufferSize;

			return true;
		}


		void Draw::destroyBufferData(BufferData* pBufferData) const {
			
			if (pBufferData->indexBufferId != UINT_MAX) {
				GLuint curBufferId{};
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBufferData->indexBufferId);
				this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

				this->_f.pGlDeleteBuffers(1, &pBufferData->indexBufferId);
				pBufferData->indexBufferId = UINT_MAX;
			}

			if (pBufferData->vertexBufferId != UINT_MAX) {
				GLuint curArrayBufferId{};
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curArrayBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, pBufferData->vertexBufferId);
				this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curArrayBufferId);

				this->_f.pGlDeleteBuffers(1, &pBufferData->vertexBufferId);
				pBufferData->vertexBufferId = UINT_MAX;
			}
			

			pBufferData->vertexBufferSize = 0ull;
			pBufferData->pLocalVertexBuffer = nullptr;
			pBufferData->indexBufferSize = 0ull;
			pBufferData->pLocalIndexBuffer = nullptr;
			pBufferData->curOffset = 0u;

			return;
		}


		bool Draw::createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const
		{
			this->_f.pGlGenBuffers(1, pId);

			if (*pId == UINT_MAX) return false;

			GLuint curBufferId = 0u;
			glGetIntegerv(binding, reinterpret_cast<GLint*>(&curBufferId));

			this->_f.pGlBindBuffer(target, *pId);
			this->_f.pGlBufferData(target, size, nullptr, GL_DYNAMIC_DRAW);
			
			this->_f.pGlBindBuffer(target, curBufferId);

			return true;
		}


		bool Draw::mapBufferData(BufferData* pBufferData) const {

			if (!pBufferData->pLocalVertexBuffer) {
				GLuint curBufferId{};
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, pBufferData->vertexBufferId);

				pBufferData->pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_f.pGlMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

				this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curBufferId);

				if (!pBufferData->pLocalVertexBuffer) return false;

			}

			if (!pBufferData->pLocalIndexBuffer) {
				GLuint curBufferId{};
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBufferData->indexBufferId);

				pBufferData->pLocalIndexBuffer = reinterpret_cast<GLuint*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

				this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

				if (!pBufferData->pLocalIndexBuffer) return false;

			}

			return true;
		}


		void Draw::copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset) const {

			if (!pBufferData->pLocalVertexBuffer || !pBufferData->pLocalIndexBuffer) return;

			const uint32_t newVertexCount = pBufferData->curOffset + count;

			if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize || newVertexCount * sizeof(GLuint) > pBufferData->indexBufferSize) {

				if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

			}

			for (uint32_t i = 0u; i < count; i++) {
				const uint32_t curIndex = pBufferData->curOffset + i;

				Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
				memcpy(&(pBufferData->pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

				pBufferData->pLocalIndexBuffer[curIndex] = curIndex;
			}

			pBufferData->curOffset += count;

			return;
		}


		bool Draw::resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const {

			if (newVertexCount <= pBufferData->curOffset) return true;

			BufferData oldBufferData = *pBufferData;

			if (!this->createBufferData(pBufferData, newVertexCount)) {
				this->destroyBufferData(pBufferData);

				return false;
			}

			if (!this->mapBufferData(pBufferData)) return false;


			if (oldBufferData.pLocalVertexBuffer) {
				memcpy(pBufferData->pLocalVertexBuffer, oldBufferData.pLocalVertexBuffer, oldBufferData.vertexBufferSize);
			}

			if (oldBufferData.pLocalIndexBuffer) {
				memcpy(pBufferData->pLocalIndexBuffer, oldBufferData.pLocalIndexBuffer, oldBufferData.indexBufferSize);
			}

			pBufferData->curOffset = oldBufferData.curOffset;

			this->destroyBufferData(&oldBufferData);

			return true;
		}


		void Draw::drawBufferData(BufferData* pBufferData, GLenum mode) const {
			GLuint curVertexBufferId{};
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curVertexBufferId));
			
			GLuint curIndexBufferId{};
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&curIndexBufferId));

			this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, pBufferData->vertexBufferId);
			this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pBufferData->indexBufferId);

			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_COLOR_ARRAY);

			// third argument describes the offset in the Vertext struct, not the actuall address of the buffer
			glVertexPointer(2, GL_FLOAT, sizeof(Vertex), nullptr);
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(Vertex), reinterpret_cast<GLvoid*>(sizeof(Vector2)));

			this->_f.pGlUnmapBuffer(GL_ARRAY_BUFFER);
			pBufferData->pLocalVertexBuffer = nullptr;

			this->_f.pGlUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
			pBufferData->pLocalIndexBuffer = nullptr;

			glDrawElements(mode, static_cast<GLsizei>(pBufferData->curOffset), GL_UNSIGNED_INT, nullptr);

			glDisableClientState(GL_COLOR_ARRAY);
			glDisableClientState(GL_VERTEX_ARRAY);

			pBufferData->curOffset = 0u;

			this->_f.pGlBindBuffer(GL_ARRAY_BUFFER, curVertexBufferId);
			this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curIndexBufferId);

			return;
		}

	}

}