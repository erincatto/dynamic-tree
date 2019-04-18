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

// Single box
struct Test1 : Test
{
	const char* GetCategory() const
	{
		return "Basic";
	}

	const char* GetName() const override
	{
		return "Single Box";
	}

	void CreateBoxes() override
	{
		Allocate(1);

		m_boxes[0].lowerBound = dtVecSet(-0.5f, -0.5f, -0.5f);
		m_boxes[0].upperBound = dtVecSet(0.5f, 0.5f, 0.5f);
	}
};

static Test1 s_test;
static int testIndex = RegisterTest(&s_test);
