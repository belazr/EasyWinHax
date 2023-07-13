#include "vecmath.h"

namespace hax {

	namespace vecmath {

		float calcPitch(Vector3 origin, Vector3 target) {
			const Vector3 diff = subtract(target, origin);
			const Vector2 diff2d = { diff.x, diff.y };

			const float aTan = atanf(diff.z / magnitude(diff2d));

			return TO_DEG(aTan);
		}


		float calcYaw(Vector3 origin, Vector3 target) {
			const Vector2 origin2d{ origin.x, origin.y };
			const Vector2 target2d{ target.x, target.y };
			// connection vector in xy plane
			const Vector2 diffXY = subtract(target2d, origin2d);

			float aTanDeg = TO_DEG(atanf(diffXY.y / diffXY.x));

			// adjustment for quadrants
			if (diffXY.x < 0.f) {

				if (diffXY.y < 0.f) {
					aTanDeg -= 180.f;
				}
				else {
					aTanDeg += 180.f;
				}

			}

			return aTanDeg;
		}


		Vector3 shiftByAngle(Vector3 origin, float pitch, float yaw, float distance) {
			const Vector3 factors{ cosf(TO_RAD(yaw)), sinf(TO_RAD(yaw)), sinf(TO_RAD(pitch)) };
			const Vector3 shifts = multiply(factors, distance);

			return add(origin, shifts);
		}


		static inline bool clipToScreen(const Vector4* clip, Vector2* screen, float windowWidth, float windowHeight);

		bool worldToScreenCol(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight) {
			// matrix-vector product, multiplying world coordinates by projection matrix (column-major) = clip coordinates
			const Vector4 clip{
				world->x * matrix[0] + world->y * matrix[4] + world->z * matrix[8] + matrix[12],
				world->x * matrix[1] + world->y * matrix[5] + world->z * matrix[9] + matrix[13],
				world->x * matrix[2] + world->y * matrix[6] + world->z * matrix[10] + matrix[14],
				world->x * matrix[3] + world->y * matrix[7] + world->z * matrix[11] + matrix[15]
			};

			if (clip.w < 0.1f) return false;

			return clipToScreen(&clip, screen, windowWidth, windowHeight);
		}


		bool worldToScreenRow(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight) {
			// matrix-vector product, multiplying world coordinates by projection matrix (row-major) = clip coordinates
			const Vector4 clip{
				world->x * matrix[0] + world->y * matrix[1] + world->z * matrix[2] + matrix[3],
				world->x * matrix[4] + world->y * matrix[5] + world->z * matrix[6] + matrix[7],
				world->x * matrix[8] + world->y * matrix[9] + world->z * matrix[10] + matrix[11],
				world->x * matrix[12] + world->y * matrix[13] + world->z * matrix[14] + matrix[15]
			};

			if (clip.w < 0.1f) return false;

			return clipToScreen(&clip, screen, windowWidth, windowHeight);
		}


		static inline bool clipToScreen(const Vector4* clip, Vector2* screen, float windowWidth, float windowHeight) {
			// perspective division -> normalized device coordinates
			const Vector3 clip3d{ clip->x, clip->y, clip->z };
			const Vector3 ndc = divide(clip3d, clip->w);

			// transform to window coordinates
			screen->x = (windowWidth / 2.f * ndc.x) + (ndc.x + windowWidth / 2.f);
			screen->y = -(windowHeight / 2.f * ndc.y) + (ndc.y + windowHeight / 2.f);

			// check if is on screen
			if (screen->x > windowWidth || screen->x < 0.f || screen->y > windowHeight || screen->y < 0.f) return false;

			return true;

		}

	}

}