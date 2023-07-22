#pragma once
#include <math.h>

// Defines low abstraction vector structures and low overhead functions to do calculations on them needed for drawing in three dimensional space.

namespace hax {

	constexpr float PI = 3.1415926535f;

	// Converts degrees to radians.
	//
	// Parameters:
	//
	// [in] degrees:
	// Degrees to convert to radians.
	// 
	// Return:
	// Radians converted from degrees.
	constexpr float degToRad(float degrees) {
		return degrees * 0.01745329f;
	}

	// Converts radians to degrees.
	//
	// Parameters:
	//
	// [in] radians:
	// Radians to convert to degrees.
	// 
	// Return:
	// degrees converted from radians.
	constexpr float radToDeg(float radians) {
		return radians * 57.29578f;
	}

	typedef struct Vector2 {
		float x, y;
	}Vector2;

	typedef struct Vector3 {
		float x, y, z;
	}Vector3;

	typedef struct Vector4 {
		float x, y, z, w;
	}Vector4;

	namespace vecmath {

		// Adds one two dimensional vector to another.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Summand.
		// 
		// [in] arg2:
		// Summand.
		// 
		// Return:
		// Sum.
		constexpr Vector2 add(Vector2 sum1, Vector2 sum2) {

			return { sum1.x + sum2.x, sum1.y + sum2.y };
		}

		// Subtracts one two dimensional vector from another.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Minuend.
		// 
		// [in] arg2:
		// Subtrahend.
		// 
		// Return:
		// Difference.
		constexpr Vector2 subtract(Vector2 min, Vector2 sub) {

			return { min.x - sub.x, min.y - sub.y };
		}

		// Multiplies one two dimensional vector by a scalar.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Factor.
		// 
		// [in] arg2:
		// Scalar factor.
		// 
		// Return:
		// Product.
		constexpr Vector2 multiply(Vector2 fac, float sFac) {

			return { fac.x * sFac, fac.y * sFac };
		}

		// Divides one two dimensional vector by a scalar.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Dividend.
		// 
		// [in] arg2:
		// Scalar divisor.
		// 
		// Return:
		// Fraction.
		constexpr Vector2 divide(Vector2 divd, float sDivs) {

			return { divd.x / sDivs, divd.y / sDivs };;
		}

		// Calculates the dot product of two two dimensional vectors.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Factor.
		// 
		// [in] arg2:
		// Factor.
		// 
		// Return:
		// Dot product.
		constexpr float dotProd(Vector2 fac1, Vector2 fac2) {

			return (fac1.x * fac2.x + fac1.y * fac2.y);
		}

		// Calculates the magnitude of a two dimensional vector.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Vector.
		// 
		// Return:
		// Magnitude of the vector.
		inline float magnitude(Vector2 vec) {

			return sqrtf(dotProd(vec, vec));
		}

		// Adds one three dimensional vector to another.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Summand.
		// 
		// [in] arg2:
		// Summand.
		// 
		// Return:
		// Sum.
		constexpr Vector3 add(Vector3 sum1, Vector3 sum2) {

			return { sum1.x + sum2.x, sum1.y + sum2.y, sum1.z + sum2.z };
		}

		// Subtracts one three dimensional vector from another.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Minuend.
		// 
		// [in] arg2:
		// Subtrahend.
		// 
		// Return:
		// Difference.
		constexpr Vector3 subtract(Vector3 min, Vector3 sub) {

			return { min.x - sub.x, min.y - sub.y, min.z - sub.z };
		}

		// Multiplies one three dimensional vector by a scalar.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Factor.
		// 
		// [in] arg2:
		// Scalar factor.
		// 
		// Return:
		// Product.
		constexpr Vector3 multiply(Vector3 fac, float sFac) {

			return { fac.x * sFac, fac.y * sFac, fac.z * sFac };
		}

		// Divides one three dimensional vector by a scalar.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Dividend.
		// 
		// [in] arg2:
		// Scalar divisor.
		// 
		// Return:
		// Fraction.
		constexpr Vector3 divide(Vector3 divd, float sDivs) {

			return { divd.x / sDivs, divd.y / sDivs, divd.z / sDivs };;
		}


		// Calculates the dot product of two three dimensional vectors.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Factor.
		// 
		// [in] arg2:
		// Factor.
		// 
		// Return:
		// Dot product.
		constexpr float dotProd(Vector3 fac1, Vector3 fac2) {

			return (fac1.x * fac2.x + fac1.y * fac2.y + fac1.z * fac2.z);
		}

		// Calculates the magnitude of a three dimensional vector.
		// 
		// Parameters:
		// 
		// [in] arg1:
		// Vector.
		// 
		// Return:
		// Magnitude of the vector.
		inline float magnitude(Vector3 vec) {

			return sqrtf(dotProd(vec, vec));
		}

		// Calculates the pitch from one point in three dimensional space to another.
		// 
		// Parameters:
		// 
		// [in] origin:
		// Origin position.
		// 
		// [in] target:
		// Target position.
		// 
		// Return:
		// Pitch in degree from -90 to 90.
		// Zero for two points with the same z value and positiv if target has greater z value than origin.
		float calcPitch(Vector3 origin, Vector3 target);

		// Calculates the yaw from on point in three dimensional space to another.
		// 
		// Parameters:
		// 
		// [in] origin:
		// Origin position.
		//
		// [in] target:
		// Target position.
		// 
		// Return:
		// Yaw in degree from -180 to 180.
		// Zero for two points with the same x value and positiv if target has greater y value than origin.
		float calcYaw(Vector3 origin, Vector3 target);

		// Calculates a point shifted by a certain angle and distance from an orgin point.
		// 
		// Parameters:
		// 
		// [in] origin:
		// Origin position.
		// 
		// [in] pitch:
		// Pitch of direction the point is shifted in.
		// 
		// [in] yaw:
		// Yaw of direction the point is shifted in.
		// 
		// [in] distance:
		// Distance the point should be shifted.
		// 
		// Return:
		// The coordinates of the shifted points.
		Vector3 shiftByAngle(Vector3 origin, float pitch, float yaw, float distance);

		// Calculates two dimensional screen coordinates from three dimensional world coordinates.
		// It uses the projection matrix of a 3d engine. The matrix has to be column major (eg OpenGL).
		// 
		// Parameters:
		// 
		// [in] world:
		// Three dimensional coordinates.
		// 
		// [out] screen:
		// Screen coordinates the point should be drawn to. In general in pixels but depending on the projection matrix.
		// 
		// [in] matrix:
		// The projection matrix of the 3d engine.
		// 
		// Return:
		// Returns true if point at the world coordinates is on screen, false if it is off screen.
		bool worldToScreenCol(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight);

		// Calculates two dimensional screen coordinates from three dimensional world coordinates.
		// It uses the projection matrix of a 3d engine. The matrix has to be row major (eg DirectX 9).
		// 
		// Parameters:
		// 
		// [in] world:
		// Three dimensional coordinates.
		// 
		// [out] screen:
		// Screen coordinates the point should be drawn to. In general in pixels but depending on the projection matrix.
		// 
		// [in] matrix:
		// The projection matrix of the 3d engine.
		// 
		// Return:
		// Returns true if point at the world coordinates is on screen, false if it is off screen.
		bool worldToScreenRow(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight);

	}

}