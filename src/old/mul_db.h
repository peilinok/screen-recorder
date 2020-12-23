#ifndef MUL_DB
#define MUL_DB

#include <math.h>

#ifdef _MSC_VER
#include <float.h>

#pragma warning(push)
#pragma warning(disable : 4056)
#pragma warning(disable : 4756)
#endif

static inline float mul_to_db(const float mul)
{
	return (mul == 0.0f) ? -INFINITY : (20.0f * log10f(mul));
}

static inline float db_to_mul(const float db)
{
	return isfinite((double)db) ? powf(10.0f, db / 20.0f) : 0.0f;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // !MUL_DB