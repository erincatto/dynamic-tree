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

struct Test3 : Test
{
	const char* GetCategory() const
	{
		return "Benchmark";
	}

	const char* GetName() const override
	{
		return "Overwatch Map";
	}

	bool Load(const char* fileName)
	{
		m_vertices = nullptr;
		m_vertexCount = 0;

		FILE* file = fopen(fileName, "r");
		if (file == nullptr)
		{
			return false;
		}

		const int k_bufferSize = 256;
		char buffer[k_bufferSize];

		while (fgets(buffer, k_bufferSize, file))
		{
			switch (buffer[0])
			{
			case 'v':
				++m_vertexCount;
				break;
			}
		}

		rewind(file);

		m_vertices = (dtVec*)malloc(m_vertexCount * sizeof(dtVec));

		int index = 0;
		while (fgets(buffer, k_bufferSize, file))
		{
			dtVec* v = m_vertices + index;
			float x, y, z;

			switch (buffer[0])
			{
			case 'v':
				sscanf(buffer, "v %f %f %f", &x, &y, &z);
				m_vertices[index] = dtVecSet(x, y, z);
				++index;
				break;
			}
		}

		fclose(file);

		return true;
	}

	void Create(dtTreeHeuristic heuristic, bool rotate) override
	{
		m_tree.m_heuristic = heuristic;
		m_rotate = rotate;

		bool success = Load("data/BlizzardLandTree.txt");
		assert(success);

		m_proxyCount = m_vertexCount / 2;
		m_proxies = (int*)malloc(m_proxyCount * sizeof(int));

		dtTimer timer;
		for (int i = 0; i < m_proxyCount; ++i)
		{
			dtAABB box;
			box.lowerBound = m_vertices[2 * i + 0];
			box.upperBound = m_vertices[2 * i + 1];
			m_proxies[i] = m_tree.CreateProxy(box, m_rotate);
		}
		m_buildTime = timer.GetMilliseconds();

		m_base = 0;
	}

	void Update(Draw& draw)
	{
		if (m_proxyCount == 0)
		{
			return;
		}

		const int count = 100;
		dtTimer timer;
		for (int i = 0; i < count; ++i)
		{
			int index = m_base;
			m_tree.DestroyProxy(m_proxies[index], m_rotate);

			dtAABB box;
			box.lowerBound = m_vertices[2 * index + 0];
			box.upperBound = m_vertices[2 * index + 1];
			m_proxies[index] = m_tree.CreateProxy(box, m_rotate);

			m_base += 1;
			if (m_base == m_proxyCount)
			{
				m_base = 0;
			}
		}
		float updateTime = timer.GetMilliseconds();

		draw.DrawString(5, 45, "build time = %6.2f, update time = %5.2f", m_buildTime, updateTime);

		int height = m_tree.GetHeight();
		float area = m_tree.GetAreaRatio();
		draw.DrawString(5, 60, "current height = %d, area = %g", height, area);
	}

	void Destroy() override
	{
		m_tree.Clear();
		free(m_vertices);
		m_vertices = nullptr;
		m_vertexCount = 0;
		free(m_proxies);
		m_proxies = nullptr;
		m_proxyCount = 0;
	}

	int m_base;
	float m_buildTime;
	int* m_proxies;
	int m_proxyCount;
	dtVec* m_vertices;
	int m_vertexCount;
	bool m_rotate;
};

static Test3 s_test;
Test* g_test3 = &s_test;