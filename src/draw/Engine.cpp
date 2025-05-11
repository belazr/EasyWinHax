#include "Engine.h"
#include <string.h>

namespace hax {

	namespace draw {

		Engine::Engine(IBackend* pBackend, Font font) :
			_pointListBuffer{}, _textureTriangleListBuffer{}, _triangleListBuffer{}, _font{ font },
			_pBackend{ pBackend }, _init{}, _frame{}, frameWidth {}, frameHeight{} {}


		TextureId Engine::loadTexture(const Color* data, uint32_t width, uint32_t height) {

			if (!this->_init) return 0ull;
			
			return this->_pBackend->loadTexture(data, width, height);
		}


		void Engine::beginFrame(void* pArg1, void* pArg2) {
			this->_pBackend->setHookArguments(pArg1, pArg2);

			if (!this->_init) {
				this->_init = this->_pBackend->initialize();

				if (!_font.textureId) {
					this->_font.textureId = this->_pBackend->loadTexture(this->_font.pTexture, this->_font.width, this->_font.height);
				}

				this->_init = _font.textureId != 0u;
			}

			if (!this->_init) return;
			
			if (!this->_pBackend->beginFrame()) return;

			this->_pBackend->getFrameResolution(&this->frameWidth, &this->frameHeight);

			if (!this->_pointListBuffer.beginFrame(this->_pBackend->getPointListBufferBackend())) {
				this->_pBackend->endFrame();

				return;
			}

			if (!this->_textureTriangleListBuffer.beginFrame(this->_pBackend->getTextureTriangleListBufferBackend())) {
				this->_pBackend->endFrame();

				return;
			}

			if (!this->_triangleListBuffer.beginFrame(this->_pBackend->getTriangleListBufferBackend())) {
				this->_pBackend->endFrame();

				return;
			}

			this->_frame = true;
			
			return;
		}


		void Engine::endFrame() {
			
			if (!this->_frame) return;
			
			this->_triangleListBuffer.endFrame();
			this->_textureTriangleListBuffer.endFrame();
			this->_pointListBuffer.endFrame();

			this->_pBackend->endFrame();

			this->_frame = false;

			return;
		}


		void Engine::drawLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) {

			if (!this->_frame) return;

			// defaults for horizontal line
			float cosAtan = 0.f;
			float sinAtan = width / 2.f;

			// trigonometry black magic to calculate the corners of the line/rectangle
			if (pos1->y - pos2->y) {
				const float omega = (pos2->x - pos1->x) / (pos1->y - pos2->y);
				const float oSqrt = sqrtf(1.f + omega * omega);

				cosAtan = width / 2.f / oSqrt;
				sinAtan = width / 2.f * omega / oSqrt;

				// has to be drawn from bottom to top
				if (pos1->y < pos2->y) {
					const Vector2* tmp = pos1;
					pos1 = pos2;
					pos2 = tmp;
				}
			}
			// horizontal has to be drawn from left to right
			else if (pos2->x < pos1->x) {
				const Vector2* const tmp = pos1;
				pos1 = pos2;
				pos2 = tmp;
			}

			const Vertex corners[]{
				{ { pos1->x - cosAtan, pos1->y - sinAtan }, color },
				{ { pos2->x - cosAtan, pos2->y - sinAtan }, color },
				{ { pos1->x + cosAtan, pos1->y + sinAtan }, color },
				{ { pos2->x + cosAtan, pos2->y + sinAtan }, color },
				{ { pos1->x + cosAtan, pos1->y + sinAtan }, color },
				{ { pos2->x - cosAtan, pos2->y - sinAtan }, color }
			};

			this->_triangleListBuffer.append(corners, _countof(corners));
		}


		void Engine::drawPLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) {
			
			if (!this->_frame) return;
			
			// only makes sense if not horizontal
			if (!(pos1->y - pos2->y)) return;

			// trigonometry black magic to calculate the corners of the line/parallelogram
			const float omega = (pos2->x - pos1->x) / (pos1->y - pos2->y);
			const float oSqrt = sqrtf(1.f + omega * omega);
			const float cosAtan = width / 2 / oSqrt;
			const float sinAtan = width / 2 * omega / oSqrt;

			// has to be drawn from bottom to top
			if (pos1->y - pos2->y < 0.f) {
				const Vector2* tmp = pos1;
				pos1 = pos2;
				pos2 = tmp;
			}

			const Vertex corners[]{
				{ { pos1->x - cosAtan - omega * sinAtan, pos1->y }, color },
				{ { pos2->x - cosAtan - omega * sinAtan, pos2->y }, color },
				{ { pos1->x + cosAtan + omega * sinAtan, pos1->y }, color },
				{ { pos2->x + cosAtan + omega * sinAtan, pos2->y }, color },
				{ { pos1->x + cosAtan + omega * sinAtan, pos1->y }, color },
				{ { pos2->x - cosAtan - omega * sinAtan, pos2->y }, color }
			};

			this->_triangleListBuffer.append(corners, _countof(corners));
		}


		void Engine::drawFilledRectangle(const Vector2* pos, float width, float height, Color color) {

			if (!this->_frame) return;

			const Vertex corners[]{
				{ { pos->x, pos->y }, color },
				{ { pos->x + width, pos->y }, color },
				{ { pos->x, pos->y + height }, color },
				{ { pos->x + width, pos->y + height }, color },
				{ { pos->x, pos->y + height }, color },
				{ { pos->x + width, pos->y }, color }
			};

			this->_triangleListBuffer.append(corners, _countof(corners));
		}


		void Engine::drawTexture(TextureId textureId, const Vector2* pos, float width, float height) {

			if (!this->_frame) return;

			const Vertex corners[]{
				{ { pos->x, pos->y }, abgr::WHITE, { 0.f, 0.f } },
				{ { pos->x + width, pos->y }, abgr::WHITE, { 1.f, 0.f }  },
				{ { pos->x, pos->y + height }, abgr::WHITE, { 0.f, 1.f }  },
				{ { pos->x + width, pos->y + height }, abgr::WHITE, { 1.f, 1.f }  },
				{ { pos->x, pos->y + height }, abgr::WHITE, { 0.f, 1.f }  },
				{ { pos->x + width, pos->y }, abgr::WHITE, { 1.f, 0.f }  }
			};

			this->_textureTriangleListBuffer.append(corners, _countof(corners), textureId);

			return;
		}


		void Engine::drawString(const Vector2* pos, const char* text, uint32_t size, Color color) {

			if (!this->_frame) return;

			const size_t length = strlen(text);
			// font textures are generated with font size 24
			const float sizeFactor = size / 24.f;

			for (size_t i = 0u; i < length; i++) {
				// default to blank for unknown chars
				const uint32_t curCharIndex = (static_cast<uint32_t>(text[i]) - 32u) > 95u ? 0u : static_cast<uint32_t>(text[i]) - 32u;
				
				// it is measurably faster to do the calculation multiple times (???) in the initialization
				// MSVC, why do you have to be like this...
				const Vertex corners[]{
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor, pos->y },
						color,
						{ this->_font.charWidth * curCharIndex / static_cast<float>(this->_font.width), 0.f}
					},
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor + this->_font.charWidth * sizeFactor, pos->y },
						color,
						{ this->_font.charWidth * (curCharIndex + 1u) / static_cast<float>(this->_font.width), 0.f }
					},
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor, pos->y + this->_font.height * sizeFactor },
						color,
						{ this->_font.charWidth * curCharIndex / static_cast<float>(this->_font.width), 1.f }
					},
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor + this->_font.charWidth * sizeFactor, pos->y + this->_font.height * sizeFactor },
						color,
						{ this->_font.charWidth * (curCharIndex + 1u) / static_cast<float>(this->_font.width), 1.f }
					},
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor, pos->y + this->_font.height * sizeFactor },
						color,
						{ this->_font.charWidth * curCharIndex / static_cast<float>(this->_font.width), 1.f }
					},
					{
						{ pos->x + i * this->_font.charWidth * sizeFactor + this->_font.charWidth * sizeFactor, pos->y },
						color,
						{ this->_font.charWidth * (curCharIndex + 1u) / static_cast<float>(this->_font.width), 0.f }
					}
				};

				this->_textureTriangleListBuffer.append(corners, _countof(corners), this->_font.textureId);
			}

			return;
		}

		Vector2 Engine::getStringDimensions(const char* text, uint32_t size) const {
			const size_t length = strlen(text);
			// font textures are generated with font size 24
			const float sizeFactor = static_cast<float>(size) / 24.f;

			return { length * this->_font.charWidth * sizeFactor, this->_font.height * sizeFactor };
		}


		void Engine::drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, Color color) {
			
			if (!this->_frame) return;
			
			const float height = bot->y - top->y;

			// corners coordinates of the parallelogram
			Vector2 bl{ bot->x - height / (2.f * ratio), bot->y };
			Vector2 br{ bot->x + height / (2.f * ratio), bot->y };
			Vector2 tl{ top->x - height / (2.f * ratio), top->y };
			Vector2 tr{ top->x + height / (2.f * ratio), top->y };

			//top
			this->drawLine(&tl, &tr, width, color);
			//bottom
			this->drawLine(&bl, &br, width, color);

			// columns
			// have to be offset by half of the line width for a clean overlap at the corners
			const float halfWidth = width / 2.f;
			tl.y -= halfWidth;
			tr.y -= halfWidth;
			bl.y += halfWidth;
			br.y += halfWidth;
			this->drawPLine(&bl, &tl, width, color);
			this->drawPLine(&br, &tr, width, color);

			return;
		}


		void Engine::draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, Color color) {

			if (!this->_frame) return;

			this->drawLine(&top[0], &top[1], width, color);
			this->drawLine(&bot[0], &bot[1], width, color);

			// columns
			// have to be offset by half of the line width for a clean overlap at the corners
			const float halfWidth = width / 2.f;

			for (int i = 0; i < 2; i++) {
				const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
				const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
				this->drawLine(&botCol, &topCol, width, color);
			}

			return;
		}


		void Engine::draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, Color color) {

			if (!this->_frame) return;

			// top face
			this->drawLine(&top[0], &top[1], width, color);
			this->drawLine(&top[1], &top[2], width, color);
			this->drawLine(&top[2], &top[3], width, color);
			this->drawLine(&top[3], &top[0], width, color);

			// bottom face
			this->drawLine(&bot[0], &bot[1], width, color);
			this->drawLine(&bot[1], &bot[2], width, color);
			this->drawLine(&bot[2], &bot[3], width, color);
			this->drawLine(&bot[3], &bot[0], width, color);

			// columns
			// have to be offset by half of the line width for a clean overlap at the corners
			const float halfWidth = width / 2.f;

			for (int i = 0; i < 4; i++) {
				const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
				const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
				this->drawLine(&botCol, &topCol, width, color);
			}

			return;
		}

	}

}