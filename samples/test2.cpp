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

#include "test.h"

// Ordered row of boxes
struct Test2 : Test
{
	const char* GetCategory() const
	{
		return "Basic";
	}

	const char* GetName() const override
	{
		return "Ordered Row";
	}

	void CreateBoxes() override
	{
		const int count = 1 << 5;

		Allocate(count);

		float x = 0.0f;
		for (int i = 0; i < count; ++i)
		{
			m_boxes[i].lowerBound = dtVecSet(x, 0.0f, 0.0f);
			m_boxes[i].upperBound = dtVecSet(x + 1.0f, 1.0f, 1.0f);
			x += 1.0f;
		}
	}
};

static Test2 s_test;
static int testIndex = RegisterTest(&s_test);
