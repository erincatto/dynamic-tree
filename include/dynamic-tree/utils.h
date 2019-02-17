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

struct dtVec { float x, y, z, w; };

static const dtVec dtVec_Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitX = { 1.0f, 0.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitY = { 0.0f, 1.0f, 0.0f, 0.0f };
static const dtVec dtVec_UnitZ = { 0.0f, 0.0f, 1.0f, 0.0f };
//static const dtVec dtQuat_Identity = { 0.0f, 0.0f, 0.0f, 1.0f };

inline dtVec dtVecSet(float x, float y, float z)
{
	dtVec v;
	v.x = x; v.y = y; v.z = z; v.w = 0.0f;
	return v;
}

inline dtVec dtVecSet(float x, float y, float z, float w)
{
	dtVec v;
	v.x = x; v.y = y; v.z = z; v.w = w;
	return v;
}

inline dtVec dtMin(const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x < b.x ? a.x : b.x;
	v.y = a.y < b.y ? a.y : b.y;
	v.z = a.z < b.z ? a.z : b.z;
	v.w = a.w < b.w ? a.w : b.w;
	return v;
}

inline dtVec dtMax(const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x > b.x ? a.x : b.x;
	v.y = a.y > b.y ? a.y : b.y;
	v.z = a.z > b.z ? a.z : b.z;
	v.w = a.w > b.w ? a.w : b.w;
	return v;
}

inline dtVec operator + (const dtVec& a, const dtVec& b)
{
	return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

inline dtVec operator - (const dtVec& a, const dtVec& b)
{
	return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

inline dtVec& operator += (dtVec& a, const dtVec& b)
{
	a.x += b.x;
	a.y += b.y;
	a.z += b.z;
	a.w += b.w;
	return a;
}

inline dtVec& operator -= (dtVec& a, const dtVec& b)
{
	a.x -= b.x;
	a.y -= b.y;
	a.z -= b.z;
	a.w -= b.w;
	return a;
}

inline dtVec operator * (float a, const dtVec& b)
{
	return { a * b.x, a * b.y, a * b.z, a * b.w };
}

inline dtVec operator - (const dtVec& a)
{
	return { -a.x, -a.y, -a.z, -a.w };
}

inline bool operator == (const dtVec& a, const dtVec& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

inline float dtDot3(const dtVec& a, const dtVec& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline dtVec dtCross(const dtVec& a, const dtVec& b)
{
	return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

inline dtVec dtNormalize3(const dtVec& v)
{
	float len = sqrtf(dtDot3(v, v));
	return { v.x / len, v.y / len, v.z / len, 0.0f };
}

#if 0
inline float dtDot4(const dtVec& a, const dtVec& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline dtVec dtNormalize4(const dtVec& v)
{
	float len = sqrtf(dtDot3(v, v));
	return { v.x / len, v.y / len, v.z / len, v.w / len };
}

inline dtVec dtConjugate(const dtVec& q)
{
	return { -q.x, -q.y, -q.z, q.w };
}

inline dtVec dtRotate(const dtVec& q, const dtVec& u)
{
	// Rotate u by q
	// v = q * u * qc
	// This simplifies to:
	// v = u + 2 * cross(q.v, cross(q.v, u) + q.w * u)
	dtVec t = dtCross(q, u) + q.w * u;
	t = dtCross(q, t);
	dtVec v = u + t + t;
	return v;
}
#endif

struct dtMtx
{
	dtVec cx, cy, cz, cw;
};

// http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
//     [1-2yy-2zz 2xy-2zw   2xz+2yw  ]
// R = [2xy+2zw   1-2xx-2zz 2yz-2xw  ]
//     [2xz-2yw   2yz+2xw   1-2xx-2yy]
inline dtMtx dtMtxFromQuat(const dtVec& q, const dtVec& p)
{
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;

	dtMtx R;
	R.cx = { 1.0f - 2.0f * y * y - 2.0f * z * z, 2.0f * x * y + 2.0f * z * w, 2.0f * x * z - 2.0f * y * w, 0.0f };
	R.cy = { 2.0f * x * y - 2.0f * z * w, 1.0f - 2.0f * x * x - 2.0f * z * z, 2.0f * y * z + 2.0f * x * w, 0.0f };
	R.cz = { 2.0f * x * z + 2.0f * y * w, 2.0f * y * z - 2.0f * x * w, 1.0f - 2.0f * x * x - 2.0f * y * y, 0.0f };
	R.cw = p;
	return R;
}

inline dtVec dtTransformVector(const dtMtx& m, const dtVec& v)
{
	return {
		m.cx.x * v.x + m.cy.x * v.y + m.cz.x * v.z,
		m.cx.y * v.x + m.cy.y * v.y + m.cz.y * v.z,
		m.cx.z * v.x + m.cy.z * v.y + m.cz.z * v.z
	};
}

inline dtVec dtTransformPoint(const dtMtx& m, const dtVec& p)
{
	return {
		m.cx.x * p.x + m.cy.x * p.y + m.cz.x * p.z + m.cw.x,
		m.cx.y * p.x + m.cy.y * p.y + m.cz.y * p.z + m.cw.y,
		m.cx.z * p.x + m.cy.z * p.y + m.cz.z * p.z + m.cw.z
	};
}

inline dtVec dtInvTransformVector(const dtMtx& m, const dtVec& v)
{
	return {
		m.cx.x * v.x + m.cx.y * v.y + m.cx.z * v.z,
		m.cy.x * v.x + m.cy.y * v.y + m.cy.z * v.z,
		m.cz.x * v.x + m.cz.y * v.y + m.cz.z * v.z
	};
}

inline dtVec dtInvTransformPoint(const dtMtx& m, const dtVec& p)
{
	dtVec r = p - m.cw;
	return {
		m.cx.x * r.x + m.cx.y * r.y + m.cx.z * r.z,
		m.cy.x * r.x + m.cy.y * r.y + m.cy.z * r.z,
		m.cz.x * r.x + m.cz.y * r.y + m.cz.z * r.z
	};
}

// y = R * x + p
// x = RT * (y - p)
//   = RT * y - RT * p
inline dtMtx dtMtx_InvertOrtho(const dtMtx& m)
{
	dtMtx im;
	im.cx = { m.cx.x, m.cy.x, m.cz.x };
	im.cy = { m.cx.y, m.cy.y, m.cz.y };
	im.cz = { m.cx.z, m.cy.z, m.cz.z };
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
	return 2.0f * (w.x * w.y + w.y * w.z + w.z * w.x);
}

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
