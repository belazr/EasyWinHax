#pragma once
#include <math.h>

// Defines low abstraction vector structures and low overhead functions to do calculations on them needed for drawing in three dimensional space.

#define PI 3.1415926535f
#define TO_RAD(x) ((x) * 0.01745329f)
#define TO_DEG(x) ((x) * 57.29578f)

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
	// Parameters:
	// [in] arg1:	Summand.
	// [in] arg2:	Summand.
	// Return:		Sum.
	inline Vector2 add(Vector2 sum1, Vector2 sum2);

	// Subtracts one two dimensional vector from another.
	// Parameters:
	// [in] arg1:	Minuend.
	// [in] arg2:	Subtrahend.
	// Return:		Difference.
	inline Vector2 subtract(Vector2 min, Vector2 sub);

	// Multiplies one two dimensional vector by a scalar.
	// Parameters:
	// [in] arg1:	Factor.
	// [in] arg2:	Scalar factor.
	// Return:		Product.
	inline Vector2 multiply(Vector2 fac, float sFac);

	// Divides one two dimensional vector by a scalar.
	// Parameters:
	// [in] arg1:	Dividend.
	// [in] arg2:	Scalar divisor.
	// Return:		Fraction.
	inline Vector2 divide(Vector2 divd, float sDivs);

	// Calculates the dot product of two two dimensional vectors.
	// Parameters:
	// [in] arg1:	Factor.
	// [in] arg2:	Factor.
	// Return:		Dot product.
	inline float dotProd(Vector2 fac1, Vector2 fac2);

	// Calculates the magnitude of a two dimensional vector.
	// Parameters:
	// [in] arg1:	Vector.
	// Return:		Magnitude of the vector.
	float magnitude(Vector2 vec);

	// Adds one three dimensional vector to another.
	// Parameters:
	// [in] arg1:	Summand.
	// [in] arg2:	Summand.
	// Return:		Sum.
	inline Vector3 add(Vector3 sum1, Vector3 sum2);
	
	// Subtracts one three dimensional vector from another.
	// Parameters:
	// [in] arg1:	Minuend.
	// [in] arg2:	Subtrahend.
	// Return:		Difference.
	inline Vector3 subtract(Vector3 min, Vector3 sub);

	// Multiplies one three dimensional vector by a scalar.
	// Parameters:
	// [in] arg1:	Factor.
	// [in] arg2:	Scalar factor.
	// Return:		Product.
	inline Vector3 multiply(Vector3 fac, float sFac);

	// Divides one three dimensional vector by a scalar.
	// Parameters:
	// [in] arg1:	Dividend.
	// [in] arg2:	Scalar divisor.
	// Return:		Fraction.
	inline Vector3 divide(Vector3 divd, float sDivs);

	// Calculates the dot product of two three dimensional vectors.
	// Parameters:
	// [in] arg1:	Factor.
	// [in] arg2:	Factor.
	// Return:		Dot product.
	inline float dotProd(Vector3 fac1, Vector3 fac2);

	// Calculates the magnitude of a three dimensional vector.
	// Parameters:
	// [in] arg1:	Vector.
	// Return:		Magnitude of the vector.
	float magnitude(Vector3 vec);


	// Calculates the pitch from one point in three dimensional space to another.
	// [in] pos1:	Origin position.
	// [in] pos2:	Target position.
	// Return:		Pitch in degree from -90 to 90.
	//				Zero for two points with the same z value and positiv if target has greater z value than origin.
	float calcPitch(Vector3 origin, Vector3 target);

	// Calculates the yaw from on point in three dimensional space to another.
	// [in] pos1:	Origin position.
	// [in] pos2:	Target position.
	// Return:		Yaw in degree from -180 to 180.
	//				Zero for two points with the same x value and positiv if target has greater y value than origin.
	float calcYaw(Vector3 origin, Vector3 target);

	// Calculates a point shifted by a certain angle and distance from an orgin point.
	// [in] origin:		Origin position.
	// [in] pitch:		Pitch of direction the point is shifted in.
	// [in] yaw:		Yaw of direction the point is shifted in.
	// [in] distance:	Distance the point should be shifted.
	// Return:			The coordinates of the shifted points.
	Vector3 shiftByAngle(Vector3 origin, float pitch, float yaw, float distance);

	// Calculates two dimensional screen coordinates from three dimensional world coordinates.
	// It uses the projection matrix of a 3d engine. The matrix has to be column major (eg OpenGL).
	// [in] world:		Three dimensional coordinates.
	// [out] screen:	Screen coordinates the point should be drawn to. In general in pixels but depending on the projection matrix.
	// [in] matrix:		The projection matrix of the 3d engine.
	// Return:			Returns true if point at the world coordinates is on screen, false if it is off screen.
	bool worldToScreenCol(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight);

	// Calculates two dimensional screen coordinates from three dimensional world coordinates.
	// It uses the projection matrix of a 3d engine. The matrix has to be row major (eg DirectX 9).
	// [in] world:		Three dimensional coordinates.
	// [out] screen:	Screen coordinates the point should be drawn to. In general in pixels but depending on the projection matrix.
	// [in] matrix:		The projection matrix of the 3d engine.
	// Return:			Returns true if point at the world coordinates is on screen, false if it is off screen.
	bool worldToScreenRow(const Vector3* world, Vector2* screen, const float matrix[16], float windowWidth, float windowHeight);

}