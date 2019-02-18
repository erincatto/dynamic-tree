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

	void Create(dtTreeHeuristic heuristic, bool rotate) override
	{
		m_tree.m_heuristic = heuristic;
		m_rotate = rotate;

		const float a = 0.1f;
		dtVec r = dtVecSet(a, a, a);

		dtTimer timer;

		// Ground
		{
			const int countY = 10;
			const int countZ = 200;

			for (int k = 0; k < countY; k++)
			{
				for (int j = 0; j < countZ; j++)
				{
					dtVec p = dtVecSet(0.0f, -a - 2.0f * a * k, 2.0f * a * j - (countZ - 1.0f) * a);
					dtAABB cube;
					cube.lowerBound = p - r;
					cube.upperBound = p + r;
					int index = m_tree.CreateProxy(cube, m_rotate);
					m_proxies.push_back(index);
				}
			}
		}

		// Pyramid
		{
			const int count = 20;
			dtVec x = dtVecSet(0.0f, a, -(count - 1.0f) * a);
			dtVec deltaX = dtVecSet(0.0f, 2.0f * a, a);
			dtVec deltaY = dtVecSet(0.0f, 0.0f, 2.0f * a);

			for (int i = 0; i < count; ++i)
			{
				dtVec y = x;

				for (int j = i; j < count; ++j)
				{
					dtAABB cube;
					cube.lowerBound = y - r;
					cube.upperBound = y + r;
					int index = m_tree.CreateProxy(cube, m_rotate);
					m_proxies.push_back(index);

					y += deltaY;
				}

				x += deltaX;
			}
		}

		m_buildTime = timer.GetMilliseconds();

		m_base = 0;
	}

	void Update(Draw& draw)
	{
		int proxyCount = int(m_proxies.size());

		if (proxyCount == 0)
		{
			return;
		}

		const int count = 0;
		dtTimer timer;
		for (int i = 0; i < count; ++i)
		{
			int index = m_base;
			dtAABB cube = m_tree.GetAABB(m_proxies[index]);
			m_tree.DestroyProxy(m_proxies[index], m_rotate);

			m_proxies[index] = m_tree.CreateProxy(cube, m_rotate);

			m_base += 1;
			if (m_base == proxyCount)
			{
				m_base = 0;
			}
		}
		float updateTime = timer.GetMilliseconds();

		draw.DrawString(5, 45, "build time = %5.2fg, update time = %4.2f", m_buildTime, updateTime);

		int height = m_tree.GetHeight();
		float area = m_tree.GetAreaRatio();
		draw.DrawString(5, 60, "current height = %d, area = %g", height, area);
	}

	void Destroy() override
	{
		m_tree.Clear();
		m_proxies.clear();
	}

	int m_base;
	float m_buildTime;
	std::vector<int> m_proxies;
	bool m_rotate;
};

static Test4 s_test;
Test* g_test4 = &s_test;