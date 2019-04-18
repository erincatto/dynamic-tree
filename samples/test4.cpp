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

#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include "draw.h"

#include <stdio.h>
#include <assert.h>
#include <vector>

struct Test4 : Test
{
	const char* GetCategory() const
	{
		return "Benchmark";
	}

	const char* GetName() const override
	{
		return "Tiled World";
	}

	void CreateBoxes() override
	{
		const float a = 0.1f;
		dtVec r = dtVecSet(a, a, a);

		const int countY = 10;
		const int countZ = 200;
		const int countP = 20;

		Allocate(countY * countZ + (countP * (countP + 1)) / 2);

		int index = 0;

		// Ground
		{
			for (int k = 0; k < countY; k++)
			{
				for (int j = 0; j < countZ; j++)
				{
					dtVec p = dtVecSet(0.0f, -a - 2.0f * a * k, 2.0f * a * j - (countZ - 1.0f) * a);
					dtAABB cube;
					cube.lowerBound = p - r;
					cube.upperBound = p + r;
					m_boxes[index++] = cube;
				}
			}
		}

		// Pyramid
		{
			dtVec x = dtVecSet(0.0f, a, -(countP - 1.0f) * a);
			dtVec deltaX = dtVecSet(0.0f, 2.0f * a, a);
			dtVec deltaY = dtVecSet(0.0f, 0.0f, 2.0f * a);

			for (int i = 0; i < countP; ++i)
			{
				dtVec y = x;

				for (int j = i; j < countP; ++j)
				{
					dtAABB cube;
					cube.lowerBound = y - r;
					cube.upperBound = y + r;
					m_boxes[index++] = cube;

					y += deltaY;
				}

				x += deltaX;
			}
		}

		assert(index == m_count);
	}
};

static Test4 s_test;
static int testIndex = RegisterTest(&s_test);
