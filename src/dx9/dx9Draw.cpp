#include "dx9Draw.h"

namespace dx9 {
	
	IDirect3DDevice9* pDevice;

	void drawPrimitiveParallelogramOutline(const Vector2* bot, const Vector2* top, float ratio, float width, D3DCOLOR color) {
		const float height = bot->y - top->y;

		// corners coordinates of the parallelogram
		Vector2 bl{ bot->x - height / (2.f * ratio), bot->y};
		Vector2 br{ bot->x + height / (2.f * ratio), bot->y };
		Vector2 tl{ top->x - height / (2.f * ratio), top->y };
		Vector2 tr{ top->x + height / (2.f * ratio), top->y };
		
		//top
		drawPrimitiveLine(&tl, &tr, width, color);
		//bottom
		drawPrimitiveLine(&bl, &br, width, color);
		
		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;
		tl.y -= halfWidth;
		tr.y -= halfWidth;
		bl.y += halfWidth;
		br.y += halfWidth;
		drawPrimitivePLine(&bl, &tl, width, color);
		drawPrimitivePLine(&br, &tr, width, color);

		return;
	}


	void drawPrimitive2DBox(const Vector2 bot[2], const Vector2 top[2], float width, D3DCOLOR color) {
		dx9::drawPrimitiveLine(&top[0], &top[1], width, color);
		dx9::drawPrimitiveLine(&bot[0], &bot[1], width, color);

		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;
		
		for (int i = 0; i < 2; i++) {
			const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
			const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
			dx9::drawPrimitiveLine(&botCol, &topCol, width, color);
		}
		
		return;
	}


	void drawPrimitive3DBox(const Vector2 bot[4], const Vector2 top[4], float width, D3DCOLOR color) {
		// top face
		dx9::drawPrimitiveLine(&top[0], &top[1], width, color);
		dx9::drawPrimitiveLine(&top[1], &top[2], width, color);
		dx9::drawPrimitiveLine(&top[2], &top[3], width, color);
		dx9::drawPrimitiveLine(&top[3], &top[0], width, color);

		// bottom face
		dx9::drawPrimitiveLine(&bot[0], &bot[1], width, color);
		dx9::drawPrimitiveLine(&bot[1], &bot[2], width, color);
		dx9::drawPrimitiveLine(&bot[2], &bot[3], width, color);
		dx9::drawPrimitiveLine(&bot[3], &bot[0], width, color);

		// columns
		// have to be offset by half of the line width for a clean overlap at the corners
		const float halfWidth = width / 2.f;
		
		for (int i = 0; i < 4; i++) {
			const Vector2 botCol{ bot[i].x, bot[i].y + halfWidth };
			const Vector2 topCol{ top[i].x, top[i].y - halfWidth };
			dx9::drawPrimitiveLine(&botCol, &topCol, width, color);
		}

		return;
	}


	HRESULT drawPrimitivePLine(const Vector2* pos1, const Vector2* pos2, float width, D3DCOLOR color) {
		
		// only makes sense if not horizontal
		if (pos1->y - pos2->y) {
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

			Vertex data[4]{};
			data[0].x = pos1->x - cosAtan - omega * sinAtan;
			data[0].y = pos1->y;
			data[0].z = 1.f;
			data[0].rhw = 1.f;
			data[0].color = color;
			data[1].x = pos2->x - cosAtan - omega * sinAtan;
			data[1].y = pos2->y;
			data[1].z = 1.f;
			data[1].rhw = 1.f;
			data[1].color = color;
			data[2].x = pos1->x + cosAtan + omega * sinAtan;
			data[2].y = pos1->y;
			data[2].z = 1.f;
			data[2].rhw = 1.f;
			data[2].color = color;
			data[3].x = pos2->x + cosAtan + omega * sinAtan;
			data[3].y = pos2->y;
			data[3].z = 1.f;
			data[3].rhw = 1.f;
			data[3].color = color;

			return pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, data, sizeof(Vertex));
		}

		return E_FAIL;
	}


	HRESULT drawPrimitiveLine(const Vector2* pos1, const Vector2* pos2, float width, D3DCOLOR color) {
		// defaults for horizontal line
		float cosAtan = 0.f;
		float sinAtan = width / 2;

		// trigonometry black magic to calculate the corners of the line/rectangle
		if (pos1->y - pos2->y) {
			const float omega = (pos2->x - pos1->x) / (pos1->y - pos2->y);
			const float oSqrt = sqrtf(1.f + omega * omega);

			cosAtan = width / 2 / oSqrt;
			sinAtan = width / 2 * omega / oSqrt;

			// has to be drawn from bottom to top
			if (pos1->y < pos2->y) {
				const Vector2* tmp = pos1;
				pos1 = pos2;
				pos2 = tmp;
			}
		}
		// horizontal has to be drawn from left to right
		else if (pos2->x < pos1->x) {
			const Vector2* tmp = pos1;
			pos1 = pos2;
			pos2 = tmp;
		}

		Vertex data[4]{};
		data[0].x = pos1->x - cosAtan;
		data[0].y = pos1->y - sinAtan;
		data[0].z = 1.f;
		data[0].rhw = 1.f;
		data[0].color = color;
		data[1].x = pos2->x - cosAtan;
		data[1].y = pos2->y - sinAtan;
		data[1].z = 1.f;
		data[1].rhw = 1.f;
		data[1].color = color;
		data[2].x = pos1->x + cosAtan;
		data[2].y = pos1->y + sinAtan;
		data[2].z = 1.f;
		data[2].rhw = 1.f;
		data[2].color = color;
		data[3].x = pos2->x + cosAtan;
		data[3].y = pos2->y + sinAtan;
		data[3].z = 1.f;
		data[3].rhw = 1.f;
		data[3].color = color;

		return pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, data, sizeof(Vertex));
	}


	HRESULT drawPrimitivePoints(const Vector2* origin, const Vector2 points[], UINT count, D3DCOLOR color) {
		Vertex* const data = new Vertex[count];

		if (data) {
			
			for (unsigned int i = 0; i < count; i++) {
				data[i].x = origin->x + points[i].x;
				data[i].y = origin->y + points[i].y;
				data[i].z = 1.f;
				data[i].rhw = 1.f;
				data[i].color = color;
			}

			HRESULT hResult = pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, count, data, sizeof(Vertex));
			delete[] data;

			return hResult;
		}

		return E_FAIL;
	}


	HRESULT drawFilledRect(const Vector2* pos, float width, float height, D3DCOLOR color) {
		const D3DRECT rect{
			static_cast<long>(pos->x), static_cast<long>(pos->y),
			static_cast<long>(pos->x) + static_cast<long>(width),
			static_cast<long>(pos->y) + static_cast<long>(height)
		};

		return pDevice->Clear(1, &rect, D3DCLEAR_TARGET, color, 0.f, 0);
	}

}