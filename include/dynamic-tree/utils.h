/*
* Copyright (c) 2019 Erin Catto http://www.box2d.org
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability
* of this software for any purpose.
* It is provided "as is" without express or implied warranty.
*/

#pragma once

#include <math.h>
#include <memory.h>
#include <immintrin.h>

static const float dtPi = 3.141592654f;

inline int dtMin(int a, int b)
{
	return a < b ? a : b;
}

inline int dtMax(int a, int b)
{
	return a > b ? a : b;
}

inline int dtClamp(int a, int b, int c)
{
	return a < b ? b : (c < a ? c : a);
}

inline int dtAbs(int a)
{
	return a > 0 ? a : -a;
}

inline float dtAbs(float a)
{
	return a > 0.0f ? a : -a;
}

template<typename T>
inline void dtSwap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
}

typedef __m128 dtVec;

static const dtVec dtVec_Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitX = { 1.0f, 0.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitY = { 0.0f, 1.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitZ = { 0.0f, 0.0f, 1.0f, 0.0f };

inline dtVec dtVecSet(float x, float y, float z)
{
	return _mm_set_ps(0.0f, z, y, x);
}

inline dtVec dtVecSet(float x, float y, float z, float w)
{
	return _mm_set_ps(w, z, y, x);
}

inline dtVec dtVecSet(dtVec& x, dtVec& y, dtVec& z)
{
	dtVec r;
	r = _mm_unpacklo_ps(x, y);	// (x, y, x, y)
	r = _mm_movelh_ps(r, z);	// (x, y, z, z)
	return r;
}

inline dtVec dtSplat(float x)
{
	return _mm_set1_ps(x);
}

inline dtVec dtSetX(const dtVec& v, float x)
{
	dtVec t = _mm_set_ss(x);
	return _mm_move_ss(v, t);
}

inline dtVec dtSetY(const dtVec& v, float y)
{
	dtVec r = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 0, 1));
	dtVec t = _mm_set_ss(y);
	r = _mm_move_ss(r, t);
	r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 2, 0, 1));
	return r;
}

inline dtVec dtSetZ(const dtVec& v, float z)
{
	dtVec r = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 1, 2));
	dtVec t = _mm_set_ss(z);
	r = _mm_move_ss(r, t);
	r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(3, 0, 1, 2));
	return r;
}

inline dtVec dtSetW(const dtVec& v, float w)
{
	dtVec r = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 2, 1, 3));
	dtVec t = _mm_set_ss(w);
	r = _mm_move_ss(r, t);
	r = _mm_shuffle_ps(r, r, _MM_SHUFFLE(0, 2, 1, 3));
	return r;
}

inline float dtGetX(const dtVec& v)
{
	float s;
	_mm_store_ss(&s, v);
	return s;
}

inline float dtGetY(const dtVec& v)
{
	dtVec t = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
	float s;
	_mm_store_ss(&s, t);
	return s;
}

inline float dtGetZ(const dtVec& v)
{
	dtVec t = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));
	float s;
	_mm_store_ss(&s, t);
	return s;
}

inline float dtGetW(const dtVec& v)
{
	dtVec t = _mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 3, 3, 3));
	float s;
	_mm_store_ss(&s, t);
	return s;
}

inline float dtGet(const dtVec& v, int index)
{
	switch (index)
	{
	case 0:
		return dtGetX(v);
	case 1:
		return dtGetY(v);
	case 2:
		return dtGetZ(v);
	default:
		return dtGetW(v);
	}
}

inline dtVec dtMin(const dtVec& a, const dtVec& b)
{
	return _mm_min_ps(a, b);
}

inline dtVec dtMax(const dtVec& a, const dtVec& b)
{
	return _mm_max_ps(a, b);
}

inline dtVec operator + (const dtVec& a, const dtVec& b)
{
	return _mm_add_ps(a, b);
}

inline dtVec operator - (const dtVec& a, const dtVec& b)
{
	return _mm_sub_ps(a, b);
}

inline dtVec& operator += (dtVec& a, const dtVec& b)
{
	a = _mm_add_ps(a, b);
	return a;
}

inline dtVec& operator -= (dtVec& a, const dtVec& b)
{
	a = _mm_sub_ps(a, b);
	return a;
}

inline dtVec operator * (float a, const dtVec& b)
{
	dtVec av = _mm_set1_ps(a);
	return _mm_mul_ps(av, b);
}

inline dtVec operator * (dtVec& a, const dtVec& b)
{
	return _mm_mul_ps(a, b);
}

inline dtVec operator - (const dtVec& a)
{
	return _mm_sub_ps(_mm_setzero_ps(), a);
}

inline dtVec dtAbs(const dtVec& a)
{
	return _mm_max_ps(a, -a);
}

inline bool operator == (const dtVec& a, const dtVec& b)
{
	dtVec t = _mm_cmpeq_ps(a, b);
	return _mm_movemask_ps(t) == 0xF;
}

inline dtVec dtDot3(const dtVec& a, const dtVec& b)
{
	dtVec t = _mm_mul_ps(a, b);
	dtVec xx = _mm_shuffle_ps(t, t, _MM_SHUFFLE(0, 0, 0, 0));
	dtVec yy = _mm_shuffle_ps(t, t, _MM_SHUFFLE(1, 1, 1, 1));
	dtVec zz = _mm_shuffle_ps(t, t, _MM_SHUFFLE(2, 2, 2, 2));
	return _mm_add_ps(_mm_add_ps(xx, yy), zz);
}

inline dtVec dtCross(const dtVec& a, const dtVec& b)
{
	// http://threadlocalmutex.com/?p=8
	dtVec a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
	dtVec b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
	dtVec c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
	return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

inline dtVec dtLength3(const dtVec& a)
{
	dtVec t = dtDot3(a, a);
	return _mm_sqrt_ps(t);
}

inline dtVec dtNormalize3(const dtVec& v)
{
	dtVec length = _mm_sqrt_ps(dtDot3(v, v));
	return _mm_div_ps(v, length);
}

struct dtMtx
{
	dtVec cx, cy, cz, cw;
};

inline dtVec dtTransformVector(const dtMtx& m, const dtVec& v)
{
	dtVec x = _mm_shuffle_ps(v, v, _MM_SHUFFLE(0, 0, 0, 0));
	dtVec y = _mm_shuffle_ps(v, v, _MM_SHUFFLE(1, 1, 1, 1));
	dtVec z = _mm_shuffle_ps(v, v, _MM_SHUFFLE(2, 2, 2, 2));

	dtVec r = _mm_mul_ps(m.cx, x);
	r = _mm_add_ps(_mm_mul_ps(m.cy, y), r);
	r = _mm_add_ps(_mm_mul_ps(m.cz, z), r);
	return r;
}

inline dtVec dtTransformPoint(const dtMtx& m, const dtVec& p)
{
	dtVec x = _mm_shuffle_ps(p, p, _MM_SHUFFLE(0, 0, 0, 0));
	dtVec y = _mm_shuffle_ps(p, p, _MM_SHUFFLE(1, 1, 1, 1));
	dtVec z = _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 2, 2, 2));

	dtVec r = _mm_mul_ps(m.cx, x);
	r = _mm_add_ps(_mm_mul_ps(m.cy, y), r);
	r = _mm_add_ps(_mm_mul_ps(m.cz, z), r);
	r = _mm_add_ps(m.cw, r);
	return r;
}

inline dtVec dtInvTransformVector(const dtMtx& m, const dtVec& v)
{
	dtVec x = dtDot3(m.cx, v);
	dtVec y = dtDot3(m.cy, v);
	dtVec z = dtDot3(m.cz, v);
	return dtVecSet(x, y, z);
}

inline dtVec dtInvTransformPoint(const dtMtx& m, const dtVec& p)
{
	dtVec v = _mm_sub_ps(p, m.cw);
	dtVec x = dtDot3(m.cx, v);
	dtVec y = dtDot3(m.cy, v);
	dtVec z = dtDot3(m.cz, v);
	return dtVecSet(x, y, z);
}

inline dtMtx dmTranspose33(const dtMtx& a)
{
	dtVec t1 = _mm_shuffle_ps(a.cx, a.cy, _MM_SHUFFLE(0, 0, 0, 0));
	dtVec t2 = _mm_shuffle_ps(a.cx, a.cy, _MM_SHUFFLE(1, 1, 1, 1));
	dtVec t3 = _mm_shuffle_ps(a.cx, a.cy, _MM_SHUFFLE(2, 2, 2, 2));

	dtMtx b;
	b.cx = _mm_shuffle_ps(t1, a.cz, _MM_SHUFFLE(3, 0, 2, 0));
	b.cy = _mm_shuffle_ps(t2, a.cz, _MM_SHUFFLE(3, 1, 2, 0));
	b.cz = _mm_shuffle_ps(t3, a.cz, _MM_SHUFFLE(3, 2, 2, 0));
	b.cw = _mm_setzero_ps();
	return b;
}

// y = R * x + p
// x = RT * (y - p)
//   = RT * y - RT * p
inline dtMtx dtMtx_InvertOrtho(const dtMtx& m)
{
	dtMtx im = dmTranspose33(m);
	im.cw = -dtTransformVector(im, m.cw);
	return im;
}

struct dtAABB
{
	dtVec lowerBound;
	dtVec upperBound;
};

inline dtAABB dtUnion(const dtAABB& a, const dtAABB& b)
{
	dtAABB c;
	c.lowerBound = dtMin(a.lowerBound, b.lowerBound);
	c.upperBound = dtMax(a.upperBound, b.upperBound);
	return c;
}

inline float dtArea(const dtAABB& a)
{
	dtVec w = a.upperBound - a.lowerBound;
	dtVec x = _mm_shuffle_ps(w, w, _MM_SHUFFLE(0, 0, 0, 0));
	dtVec y = _mm_shuffle_ps(w, w, _MM_SHUFFLE(1, 1, 1, 1));
	dtVec z = _mm_shuffle_ps(w, w, _MM_SHUFFLE(2, 2, 2, 2));
	dtVec area = x * y + y * z + z * x;
	area = area + area;

	float s;
	_mm_store_ss(&s, area);
	return s;
}

inline dtVec dtCenter(const dtAABB& a)
{
	return dtSplat(0.5f) * (a.lowerBound + a.upperBound);
}

inline dtVec dtExtent(const dtAABB& a)
{
	return dtSplat(0.5f) * (a.upperBound - a.lowerBound);
}

struct dtFree
{
	void operator()(void* x) { free(x); }
};

class dtTimer
{
public:

	/// Constructor
	dtTimer();

	/// Reset the timer.
	void Reset();

	/// Get the time since construction or the last reset.
	float GetMilliseconds() const;

private:

#if defined(_WIN32)
	double m_start;
	static double s_invFrequency;
#endif
};
