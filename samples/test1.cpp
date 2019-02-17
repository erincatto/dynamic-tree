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

	void Create(dtTreeHeuristic heuristic, bool rotate) override
	{
		m_tree.m_heuristic = heuristic;

		dtAABB b;
		b.lowerBound = dtVecSet(-0.5f, -0.5f, -0.5f);
		b.upperBound = dtVecSet(0.5f, 0.5f, 0.5f);
		m_tree.CreateProxy(b, rotate);
	}

	void Destroy() override
	{
		m_tree.Clear();
	}
};

static Test1 s_test;
Test* g_test1 = &s_test;