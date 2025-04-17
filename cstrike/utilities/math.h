#pragma once

#include "../common.h"
// used: std::is_integral_v
#include <type_traits>
// used: sin, cos, pow, abs, sqrt
#include <corecrt_math.h>// used: MATH::Sin, cos, MATH::Pow, abs, sqrt
// used: rand, srand
#include <cstdlib>
// used: time
#include <ctime>


#include "../sdk/datatypes/vector.h"



// convert angle in degrees to radians
#define M_DEG2RAD(DEGREES) ((DEGREES) * (MATH::_PI / 180.f))
// convert angle in radians to degrees
#define M_RAD2DEG(RADIANS) ((RADIANS) * (180.f / MATH::_PI))
/// linearly interpolate the value between @a'X0' and @a'X1' by @a'FACTOR'
#define M_LERP(X0, X1, FACTOR) ((X0) + ((X1) - (X0)) * (FACTOR))
/// trigonometry
#define M_COS(ANGLE) cos(ANGLE)
#define M_SIN(ANGLE) sin(ANGLE)
#define M_TAN(ANGLE) tan(ANGLE)
/// power
#define M_POW(BASE, EXPONENT) pow(BASE, EXPONENT)
/// absolute value
#define M_ABS(VALUE) abs(VALUE)
/// square root
#define M_SQRT(VALUE) sqrt(VALUE)
/// floor
#define M_FLOOR(VALUE) floor(VALUE)


/*
 * MATHEMATICS
 * - basic trigonometry, algebraic mathematical functions and constants
 */
namespace MATH
{
	/* @section: constants */
	// pi value
	inline constexpr float _PI = 3.141592654f;
	// double of pi
	inline constexpr float _2PI = 6.283185307f;
	// half of pi
	inline constexpr float _HPI = 1.570796327f;
	// quarter of pi
	inline constexpr float _QPI = 0.785398163f;
	// reciprocal of double of pi
	inline constexpr float _1DIV2PI = 0.159154943f;
	// golden ratio
	inline constexpr float _PHI = 1.618033988f;

	// capture game's exports
	bool Setup();

	/* @section: algorithm */
	/// alternative of 'std::min'
	/// @returns : minimal value of the given comparable values
	template <typename T>
	[[nodiscard]] CS_INLINE constexpr const T& Min(const T& left, const T& right) noexcept
	{
		return (right < left) ? right : left;
	}

	/// alternative of 'std::max'
	/// @returns : maximal value of the given comparable values
	template <typename T>
	[[nodiscard]] CS_INLINE constexpr const T& Max(const T& left, const T& right) noexcept
	{
		return (right > left) ? right : left;
	}

	/// alternative of 'std::clamp'
	/// @returns : value clamped in range ['minimal' .. 'maximal']
	template <typename T>
	[[nodiscard]] CS_INLINE constexpr const T& Clamp(const T& value, const T& minimal, const T& maximal) noexcept
	{
		return (value < minimal) ? minimal : (value > maximal) ? maximal :
																 value;
	}

	/* @section: exponential */
	/// @returns: true if given number is power of two, false otherwise
	template <typename T> requires (std::is_integral_v<T>)
	[[nodiscard]] CS_INLINE constexpr bool IsPowerOfTwo(const T value) noexcept
	{
		return value != 0 && (value & (value - 1)) == 0;
	}

	
	inline void AngleVectors(const Vector_t& angles, Vector_t* forward, Vector_t* right, Vector_t* up)
	{
		float angle;
		static float sr, sp, sy, cr, cp, cy;

		angle = angles.y * (3.14159265358979323846264338327950288f * 2 / 360);
		sy = sin(angle);
		cy = cos(angle);

		angle = angles.x * (3.14159265358979323846264338327950288f * 2 / 360);
		sp = sin(angle);
		cp = cos(angle);

		angle = angles.z * (3.14159265358979323846264338327950288f * 2 / 360);
		sr = sin(angle);
		cr = cos(angle);

		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;

		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;

		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}

	inline void AngleVectors(Vector_t angles, Vector_t& forward, Vector_t& right, Vector_t& up)
	{
		float angle;
		static float sr, sp, sy, cr, cp, cy;

		angle = angles.y * (MATH::_2PI / 360);
		sy = sin(angle);
		cy = cos(angle);

		angle = angles.x * (MATH::_2PI / 360);
		sp = sin(angle);
		cp = cos(angle);

		angle = angles.z * (MATH::_2PI / 360);
		sr = sin(angle);
		cr = cos(angle);

		forward.x = cp * cy;
		forward.y = cp * sy;
		forward.z = -sp;

		right.x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right.y = (-1 * sr * sp * sy + -1 * cr * cy);
		right.z = -1 * sr * cp;

		up.x = (cr * sp * cy + -sr * -sy);
		up.y = (cr * sp * sy + -sr * cy);
		up.z = cr * cp;
	}

	inline void AngleVectors(Vector_t& angles, Vector_t* forward)
	{
		float sp, sy, cp, cy;

		sy = sin(M_DEG2RAD(angles[1]));
		cy = cos(M_DEG2RAD(angles[1]));

		sp = sin(M_DEG2RAD(angles[0]));
		cp = cos(M_DEG2RAD(angles[0]));

		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}



	inline float Normalize(float yaw)
	{
		while (yaw > 180.0f)
			yaw -= 360.0f;

		while (yaw < -180.0f)
			yaw += 360.0f;

		return yaw;
	}

	/* @section: random using game's exports */
	inline int(CS_CDECL* fnRandomSeed)(int iSeed) = nullptr;
	inline float(CS_CDECL* fnRandomFloat)(float flMinValue, float flMaxValue) = nullptr;
	inline float(CS_CDECL* fnRandomFloatExp)(float flMinValue, float flMaxValue, float flExponent) = nullptr;
	inline int(CS_CDECL* fnRandomInt)(int iMinValue, int iMaxValue) = nullptr;
	inline float(CS_CDECL* fnRandomGaussianFloat)(float flMean, float flStdDev) = nullptr;
}
