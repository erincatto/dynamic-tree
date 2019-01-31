/*
* Copyright (c) 2019 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <math.h>

inline int dtMax(int a, int b)
{
	return a > b ? a : b;
}

inline int dtAbs(int a)
{
	return a > 0 ? a : -a;
}

struct dtVec { float x, y, z; };

inline dtVec dtVecSet(float x, float y, float z)
{
	dtVec v;
	v.x = x; v.y = y; v.z = z;
	return v;
}

inline dtVec dtMin(const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x < b.x ? a.x : b.x;
	v.y = a.y < b.y ? a.y : b.y;
	v.z = a.z < b.z ? a.z : b.z;
	return v;
}

inline dtVec dtMax(const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x > b.x ? a.x : b.x;
	v.y = a.y > b.y ? a.y : b.y;
	v.z = a.z > b.z ? a.z : b.z;
	return v;
}

inline dtVec operator + (const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x + b.x;
	v.y = a.y + b.y;
	v.z = a.z + b.z;
	return v;
}

inline dtVec operator - (const dtVec& a, const dtVec& b)
{
	dtVec v;
	v.x = a.x - b.x;
	v.y = a.y - b.y;
	v.z = a.z - b.z;
	return v;
}

inline dtVec operator * (float a, const dtVec& b)
{
	dtVec v;
	v.x = a * b.x;
	v.y = a * b.y;
	v.z = a * b.z;
	return v;
}

inline bool operator == (const dtVec& a, const dtVec& b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
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
