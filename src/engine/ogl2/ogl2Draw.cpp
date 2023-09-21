#include "ogl2Draw.h"
#include "ogl2Font.h"
#include "..\Engine.h"
#include <Windows.h>

namespace hax {

	namespace ogl2 {

		Draw::Draw() : _hGameContext{}, _hHookContext{}, _width{}, _height{},
			_pglGenBuffers{}, _pglBindBuffer{}, _pglBufferData{}, _pglMapBuffer{},
			_pglUnmapBuffer{}, _pglDeleteBuffers {}, _pglCreateShader{}, _pglShaderSource{}, _pglCompileShader{},
			_triangleListBufferData{}, _isInit{} {}


		Draw::~Draw() {

			if (this->_triangleListBufferData.indexBufferId) {
				this->_pglDeleteBuffers(1, &this->_triangleListBufferData.indexBufferId);
			}
			
			if (this->_triangleListBufferData.vertexBufferId) {
				this->_pglDeleteBuffers(1, &this->_triangleListBufferData.vertexBufferId);
			}

			if (this->_hHookContext) {
				wglDeleteContext(this->_hHookContext);
			}

		}


		constexpr GLenum GL_ARRAY_BUFFER = 0x8892;
		constexpr GLenum GL_ELEMENT_ARRAY_BUFFER = 0x8893;

		void Draw::beginDraw(Engine* pEngine) {
			const HDC hDc = reinterpret_cast<HDC>(pEngine->pHookArg);

			if (!this->_isInit) {
				this->_pglGenBuffers = reinterpret_cast<tglGenBuffers>(wglGetProcAddress("glGenBuffers"));
				this->_pglBindBuffer = reinterpret_cast<tglBindBuffer>(wglGetProcAddress("glBindBuffer"));
				this->_pglBufferData = reinterpret_cast<tglBufferData>(wglGetProcAddress("glBufferData"));
				this->_pglMapBuffer = reinterpret_cast<tglMapBuffer>(wglGetProcAddress("glMapBuffer"));
				this->_pglUnmapBuffer = reinterpret_cast<tglUnmapBuffer>(wglGetProcAddress("glUnmapBuffer"));
				this->_pglDeleteBuffers = reinterpret_cast<tglDeleteBuffers>(wglGetProcAddress("glDeleteBuffers"));
				this->_pglCreateShader = reinterpret_cast<tglCreateShader>(wglGetProcAddress("glCreateShader"));
				this->_pglShaderSource = reinterpret_cast<tglShaderSource>(wglGetProcAddress("glShaderSource"));
				this->_pglCompileShader = reinterpret_cast<tglCompileShader>(wglGetProcAddress("glCompileShader"));

				if (!_pglGenBuffers || !_pglBindBuffer || !_pglBufferData || !_pglDeleteBuffers) return;

				this->_hGameContext = wglGetCurrentContext();
				this->_hHookContext = wglCreateContext(hDc);
				
				if (!this->_hGameContext || !this->_hHookContext) return;

				if (!wglMakeCurrent(hDc, this->_hHookContext)) return;

				constexpr GLsizei INITIAL_TRIANGLE_LIST_BUFFER_SIZE = sizeof(Vertex) * 100;

				if (!this->createBufferData(&this->_triangleListBufferData, INITIAL_TRIANGLE_LIST_BUFFER_SIZE)) return;
				
				this->_isInit = true;
			}

			wglMakeCurrent(hDc, this->_hHookContext);

			constexpr GLenum GL_WRITE_ONLY = 0x88B9;

			this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_triangleListBufferData.indexBufferId);
			this->_triangleListBufferData.pLocalIndexBuffer = reinterpret_cast<GLuint*>(this->_pglMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

			this->_pglBindBuffer(GL_ARRAY_BUFFER, this->_triangleListBufferData.vertexBufferId);
			this->_triangleListBufferData.pLocalVertexBuffer = reinterpret_cast<Vertex*>(this->_pglMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

			GLint viewport[4]{};
			glGetIntegerv(GL_VIEWPORT, viewport);

			if (this->_width == viewport[2] && this->_height == viewport[3]) return;

			this->_width = viewport[2];
			this->_height = viewport[3];
			
			pEngine->setWindowSize(this->_width, this->_height);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, static_cast<double>(this->_width), static_cast<double>(this->_height), 0, 1, -1);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glClearColor(0.f, 0.f, 0.f, 1.f);

			return;
		}


		void Draw::endDraw(const Engine* pEngine) {

			if (!this->_isInit) return;

			glEnableClientState(GL_VERTEX_ARRAY);

			// vertex buffer currently bound
			this->_pglUnmapBuffer(GL_ARRAY_BUFFER);
			glVertexPointer(2, GL_FLOAT, 0, NULL);

			this->_pglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->_triangleListBufferData.indexBufferId);
			this->_pglUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

			glDrawElements(GL_TRIANGLES, this->_triangleListBufferData.curOffset, GL_UNSIGNED_INT, nullptr);
			this->_triangleListBufferData.curOffset = 0;

			wglMakeCurrent(reinterpret_cast<HDC>(pEngine->pHookArg), this->_hGameContext);

			return;
		}

		void Draw::drawTriangleList(const Vector2 corners[], UINT count, rgb::Color color) {
			
			if (!this->_isInit || count % 3) return;
			
			//glColor3ub(UCHAR_R(color), UCHAR_G(color), UCHAR_B(color));
			//glBegin(GL_TRIANGLES);

			//for (UINT i = 0; i < count; i++) {
			//	glVertex2f(corners[i].x, corners[i].y);
			//}

			//glEnd();

			const UINT sizeNeeded = (this->_triangleListBufferData.curOffset + count) * sizeof(Vertex);

			//if (sizeNeeded > pVertexBufferData->size) {

			//	if (!this->resizeVertexBuffer(pVertexBufferData, sizeNeeded * 2)) return;

			//}

			for (UINT i = 0; i < count; i++) {
				const UINT curIndex = this->_triangleListBufferData.curOffset + i;

				Vertex curVertex({ corners[i].x, corners[i].y });
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


		const char* vertexShader{ R"(
			uniform mat4 LProjectionMatrix;
			uniform mat4 LModelViewMatrix;

			#if __VERSION__ >= 130

			in vec2 LVertexPos2D;
			in vec3 LMultiColor;

			out vec4 multiColor;

			#else

			attribute vec2 LVertexPos2D;

			attribute vec3 LMultiColor;
			varying vec4 multiColor;

			#endif

			void main()
			{
				multiColor = vec4( LMultiColor.r, LMultiColor.g, LMultiColor.b, 1.0 );
				gl_Position = LProjectionMatrix * LModelViewMatrix * vec4( LVertexPos2D.x, LVertexPos2D.y, 0.0, 1.0 );
			}
		)" };

		const char* fragmentShader{ R"(
			#if __VERSION__ >= 130

			in vec4 multiColor;
			out vec4 gl_FragColor;

			#else

			varying vec4 multiColor;

			#endif

			void main()
			{
				gl_FragColor = multiColor;
			}
		)" };

		bool Draw::compileShaders() {
			constexpr GLenum GL_CURRENT_PROGRAM = 0x8B8D;
			GLint programId{};
			glGetIntegerv(GL_CURRENT_PROGRAM, &programId);

			constexpr GLenum GL_VERTEX_SHADER = 0x8B31;
			const GLuint vertexShaderId = this->_pglCreateShader(GL_VERTEX_SHADER);
			this->_pglShaderSource(vertexShaderId, 1, reinterpret_cast<const char**>(&vertexShader), NULL);
			this->_pglCompileShader(vertexShaderId);

			constexpr GLenum GL_FRAGMENT_SHADER = 0x8B30;
			const GLuint fragmentShaderId = this->_pglCreateShader(GL_FRAGMENT_SHADER);
			this->_pglShaderSource(fragmentShaderId, 1, reinterpret_cast<const char**>(&fragmentShader), NULL);
			this->_pglCompileShader(fragmentShaderId);

			//glAttachShader(mProgramID, fragmentShader);
			//glLinkProgram(mProgramID);

			return true;
		}


		bool Draw::createBufferData(BufferData* pBufferData, GLsizei size) const {
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

	}

}