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

	void Create(dtTreeHeuristic heuristic, bool rotate) override
	{
		m_tree.m_heuristic = heuristic;
		float x = 0.0f;
		for (int i = 0; i < 10; ++i)
		{
			dtAABB box;
			box.lowerBound = dtVecSet(x, 0.0f, 0.0f);
			box.upperBound = dtVecSet(x + 1.0f, 1.0f, 1.0f);
			m_tree.CreateProxy(box, rotate);
			x += 1.0f;
		}
	}

	void Destroy() override
	{
		m_tree.Clear();
	}
};

static Test2 s_test;
Test* g_test2 = &s_test;