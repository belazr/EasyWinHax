#include "ogl2Backend.h"
#include "..\font\Font.h"
#include <Windows.h>

namespace hax {

	namespace draw {

		namespace ogl2 {

			Backend::Backend() : _f{}, _viewport{}, _triangleListBufferData{ UINT_MAX, UINT_MAX }, _pointListBufferData{ UINT_MAX, UINT_MAX } {}


			Backend::~Backend() {
				this->destroyBufferData(&this->_pointListBufferData);
				this->destroyBufferData(&this->_triangleListBufferData);
			}


			void Backend::setHookArguments(void* pArg1, const void* pArg2, void* pArg3) {
				UNREFERENCED_PARAMETER(pArg1);
				UNREFERENCED_PARAMETER(pArg2);
				UNREFERENCED_PARAMETER(pArg3);

				return;
			}


			bool Backend::initialize() {

				if (!this->getProcAddresses()) return false;

				this->destroyBufferData(&this->_triangleListBufferData);

				constexpr uint32_t INITIAL_TRIANGLE_LIST_BUFFER_SIZE = 99u;

				if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return false;

				this->destroyBufferData(&this->_pointListBufferData);

				constexpr uint32_t INITIAL_POINT_LIST_BUFFER_SIZE = 1000u;

				if (!this->createBufferData(&this->_pointListBufferData, INITIAL_POINT_LIST_BUFFER_SIZE)) return false;

				return true;
			}


			bool Backend::beginFrame() {
				
				if (!this->mapBufferData(&this->_triangleListBufferData)) return false;

				if (!this->mapBufferData(&this->_pointListBufferData)) return false;
				
				glGetIntegerv(GL_VIEWPORT, this->_viewport);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				glOrtho(0, static_cast<double>(this->_viewport[2]), static_cast<double>(this->_viewport[3]), 0, -1.f, 1.f);

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();

				return true;
			}


			void Backend::endFrame() {
				this->drawBufferData(&this->_pointListBufferData, GL_POINTS);
				this->drawBufferData(&this->_triangleListBufferData, GL_TRIANGLES);

				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();

				glMatrixMode(GL_PROJECTION);
				glPopMatrix();

				return;
			}


			void Backend::getFrameResolution(float* frameWidth, float* frameHeight) {
				*frameWidth = static_cast<float>(this->_viewport[2]);
				*frameHeight = static_cast<float>(this->_viewport[3]);

				return;
			}


			void Backend::drawTriangleList(const Vector2 corners[], uint32_t count, rgb::Color color) {

				if (count % 3u) return;

				this->copyToBufferData(&this->_triangleListBufferData, corners, count, color);

				return;
			}


			void Backend::drawPointList(const Vector2 coordinates[], uint32_t count, rgb::Color color, Vector2 offset) {
				this->copyToBufferData(&this->_pointListBufferData, coordinates, count, color, offset);

				return;
			}


			#define ASSIGN_PROC_ADDRESS(f) this->_f.pGl##f = reinterpret_cast<tGl##f>(wglGetProcAddress("gl"#f))

			bool Backend::getProcAddresses() {
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


			bool Backend::createBufferData(BufferData* pBufferData, uint32_t vertexCount) const {
				RtlSecureZeroMemory(pBufferData, sizeof(BufferData));

				const uint32_t vertexBufferSize = vertexCount * sizeof(Vertex);

				if (!this->createBuffer(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING, vertexBufferSize, &pBufferData->vertexBufferId)) return false;

				pBufferData->vertexBufferSize = vertexBufferSize;

				const uint32_t indexBufferSize = vertexCount * sizeof(GLuint);

				if (!this->createBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING, indexBufferSize, &pBufferData->indexBufferId)) return false;

				pBufferData->indexBufferSize = indexBufferSize;

				return true;
			}


			void Backend::destroyBufferData(BufferData* pBufferData) const {

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


			bool Backend::createBuffer(GLenum target, GLenum binding, uint32_t size, GLuint* pId) const {
				this->_f.pGlGenBuffers(1, pId);

				if (*pId == UINT_MAX) return false;

				GLuint curBufferId = 0u;
				glGetIntegerv(binding, reinterpret_cast<GLint*>(&curBufferId));

				this->_f.pGlBindBuffer(target, *pId);
				this->_f.pGlBufferData(target, size, nullptr, GL_DYNAMIC_DRAW);

				this->_f.pGlBindBuffer(target, curBufferId);

				return true;
			}


			bool Backend::mapBufferData(BufferData* pBufferData) const {

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

					pBufferData->pLocalIndexBuffer = reinterpret_cast<uint32_t*>(this->_f.pGlMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

					this->_f.pGlBindBuffer(GL_ELEMENT_ARRAY_BUFFER, curBufferId);

					if (!pBufferData->pLocalIndexBuffer) return false;

				}

				return true;
			}


			void Backend::copyToBufferData(BufferData* pBufferData, const Vector2 data[], uint32_t count, rgb::Color color, Vector2 offset) const {
				const uint32_t newVertexCount = pBufferData->curOffset + count;

				if (newVertexCount * sizeof(Vertex) > pBufferData->vertexBufferSize || newVertexCount * sizeof(uint32_t) > pBufferData->indexBufferSize) {

					if (!this->resizeBufferData(pBufferData, newVertexCount * 2u)) return;

				}

				if (!pBufferData->pLocalVertexBuffer || !pBufferData->pLocalIndexBuffer) return;

				for (uint32_t i = 0u; i < count; i++) {
					const uint32_t curIndex = pBufferData->curOffset + i;

					const Vertex curVertex{ { data[i].x + offset.x, data[i].y + offset.y }, color };
					memcpy(&(pBufferData->pLocalVertexBuffer[curIndex]), &curVertex, sizeof(Vertex));

					pBufferData->pLocalIndexBuffer[curIndex] = curIndex;
				}

				pBufferData->curOffset += count;

				return;
			}


			bool Backend::resizeBufferData(BufferData* pBufferData, uint32_t newVertexCount) const {

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


			void Backend::drawBufferData(BufferData* pBufferData, GLenum mode) const {
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

}