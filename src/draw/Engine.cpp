#include "Engine.h"
#include <string.h>

namespace hax {

	namespace draw {

		Engine::Engine(IBackend* pBackend) : _pBackend{ pBackend }, _init{}, _frame{},
			_pTriangleListBuffer{}, _pPointListBuffer{}, frameWidth {}, frameHeight{} {}


		void* Engine::loadTexture(const Color* data, uint32_t width, uint32_t height) {

			if (!this->_init) return nullptr;
			
			return this->_pBackend->loadTexture(data, width, height);
		}


		void Engine::beginFrame(void* pArg1, void* pArg2) {
			this->_pBackend->setHookArguments(pArg1, pArg2);

			if (!this->_init) {
				this->_init = this->_pBackend->initialize();
			}

			if (!this->_init) return;
			
			if (!this->_pBackend->beginFrame()) return;
			
			this->_pTriangleListBuffer = this->_pBackend->getTriangleListBuffer();
			this->_pPointListBuffer = this->_pBackend->getPointListBuffer();

			if (!this->_pTriangleListBuffer->map()) {
				this->_pBackend->endFrame();

				return;
			}

			if (!this->_pPointListBuffer->map()) {
				this->_pBackend->endFrame();

				return;
			}

			this->_pBackend->getFrameResolution(&this->frameWidth, &this->frameHeight);

			this->_frame = true;
			
			return;
		}


		void Engine::endFrame() {
			
			if (!this->_frame) return;
			
			this->_pTriangleListBuffer->draw();
			this->_pTriangleListBuffer = nullptr;
			
			this->_pPointListBuffer->draw();
			this->_pPointListBuffer = nullptr;

			this->_pBackend->endFrame();

			this->_frame = false;

			return;
		}


		void Engine::drawLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) const {

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

			const Vector2 corners[]{
				{ pos1->x - cosAtan, pos1->y - sinAtan },
				{ pos2->x - cosAtan, pos2->y - sinAtan },
				{ pos1->x + cosAtan, pos1->y + sinAtan },
				{ pos2->x + cosAtan, pos2->y + sinAtan },
				{ pos1->x + cosAtan, pos1->y + sinAtan },
				{ pos2->x - cosAtan, pos2->y - sinAtan }
			};

			this->_pTriangleListBuffer->append(corners, 6u, color);
		}


		void Engine::drawPLine(const Vector2* pos1, const Vector2* pos2, float width, Color color) const {
			
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

			const Vector2 corners[]{
				{ pos1->x - cosAtan - omega * sinAtan, pos1->y },
				{ pos2->x - cosAtan - omega * sinAtan, pos2->y },
				{ pos1->x + cosAtan + omega * sinAtan, pos1->y },
				{ pos2->x + cosAtan + omega * sinAtan, pos2->y },
				{ pos1->x + cosAtan + omega * sinAtan, pos1->y },
				{ pos2->x - cosAtan - omega * sinAtan, pos2->y }
			};

			this->_pTriangleListBuffer->append(corners, 6, color);
		}


		void Engine::drawFilledRectangle(const Vector2* pos, float width, float height, Color color) const {

			if (!this->_frame) return;

			const Vector2 corners[]{
				{ pos->x, pos->y },
				{ pos->x + width, pos->y },
				{ pos->x, pos->y + height },
				{ pos->x + width, pos->y + height },
				{ pos->x, pos->y + height },
				{ pos->x + width, pos->y }
			};

			this->_pTriangleListBuffer->append(corners, 6u, color);
		}


		void Engine::drawTexture(void* id, const Vector2* pos, float width, float height) const {

			if (!this->_frame) return;

			const Vector2 corners[]{
				{ pos->x, pos->y },
				{ pos->x + width, pos->y },
				{ pos->x, pos->y + height },
				{ pos->x + width, pos->y + height },
				{ pos->x, pos->y + height },
				{ pos->x + width, pos->y }
			};

			this->_pTriangleListBuffer->append(corners, _countof(corners), static_cast<Color>(0), {}, id);

			return;
		}


		void Engine::drawString(const font::Font* pFont, const Vector2* pos, const char* text, Color color) const {

			if (!this->_frame) return;

			const size_t size = strlen(text);

			for (size_t i = 0; i < size; i++) {
				const char c = text[i];

				if (c == ' ') continue;

				const font::CharIndex index = font::charToCharIndex(c);
				const font::Char* pCurChar = &pFont->chars[index];

				if (pCurChar) {
					// current char x coordinate is offset by width of previously drawn chars plus two pixels spacing per char
					const Vector2 curPos{ pos->x + (pFont->width + 2.f) * i, pos->y - pFont->height };
					this->_pPointListBuffer->append(pCurChar->body.coordinates, pCurChar->body.count, color, curPos);
					this->_pPointListBuffer->append(pCurChar->outline.coordinates, pCurChar->outline.count, abgr::BLACK, curPos);
				}

			}

			return;
		}


		void Engine::drawParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, Color color) const {
			
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


		void Engine::draw2DBox(const Vector2 bot[2], const Vector2 top[2], float width, Color color) const {

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


		void Engine::draw3DBox(const Vector2 bot[4], const Vector2 top[4], float width, Color color) const {

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