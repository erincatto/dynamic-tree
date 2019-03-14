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

void Test::Create(dtInsertionHeuristic heuristic)
{
	CreateBoxes();

	m_tree.m_heuristic = heuristic;

	dtTimer timer;
	for (int i = 0; i < m_count; ++i)
	{
		m_proxies[i] = m_tree.CreateProxy(m_boxes[i], i);
	}
	m_buildTime = timer.GetMilliseconds();

	m_proxyCount = 0;
	m_nodeCount = 0;
	m_treeHeight = 0;
	m_heapCount = 0;
	m_treeArea = 0.0f;

	m_proxyCount = m_tree.GetProxyCount();
	m_nodeCount = m_tree.m_nodeCount;
	m_treeHeight = m_tree.GetHeight();
	m_heapCount = m_tree.m_maxHeapCount;
	m_treeArea = m_tree.GetAreaRatio();

	m_base = 0;
}

void Test::Update(Draw& draw, int reinsertIter, int shuffleIter)
{
	if (m_count == 0)
	{
		return;
	}

	dtTimer timer;
	for (int i = 0; i < reinsertIter; ++i)
	{
		int index = m_base;
		m_tree.DestroyProxy(m_proxies[index]);

		m_proxies[index] = m_tree.CreateProxy(m_boxes[index], i);

		m_base += 1;
		if (m_base == m_count)
		{
			m_base = 0;
		}
	}

	m_tree.Optimize(shuffleIter);

	float updateTime = timer.GetMilliseconds();

	draw.DrawString(5, 50, "build time = %6.2f, update time = %8.2f us", m_buildTime, 1000.0f * updateTime);

	int height = m_tree.GetHeight();
	float area = m_tree.GetAreaRatio();
	draw.DrawString(5, 65, "current height = %d, area = %g", height, area);
}

void Test::Destroy()
{
	m_tree.Clear();
	free(m_boxes);
	free(m_proxies);
	m_boxes = nullptr;
	m_proxies = nullptr;
	m_count = 0;
}

void Test::Allocate(int count)
{
	m_boxes = (dtAABB*)malloc(count * sizeof(dtAABB));
	m_proxies = (int*)malloc(count * sizeof(int));
	m_count = count;
}

void Test::RebuildTopDownSAH()
{
	m_tree.Clear();

	dtTimer timer;
	m_tree.BuildTopDownSAH(m_proxies, m_boxes, m_count);
	m_buildTime = timer.GetMilliseconds();

	m_proxyCount = m_tree.GetProxyCount();
	m_nodeCount = m_tree.m_nodeCount;
	m_treeHeight = m_tree.GetHeight();
	m_heapCount = m_tree.m_maxHeapCount;
	m_treeArea = m_tree.GetAreaRatio();

	m_base = 0;
}

void Test::RebuildTopDownMedian()
{
	m_tree.Clear();

	dtTimer timer;
	m_tree.BuildTopDownMedianSplit(m_proxies, m_boxes, m_count);
	m_buildTime = timer.GetMilliseconds();

	m_proxyCount = m_tree.GetProxyCount();
	m_nodeCount = m_tree.m_nodeCount;
	m_treeHeight = m_tree.GetHeight();
	m_heapCount = m_tree.m_maxHeapCount;
	m_treeArea = m_tree.GetAreaRatio();

	m_base = 0;
}

void Test::RebuildBottomUp()
{
	dtTimer timer;
	m_tree.RebuildBottomUp();
	m_buildTime = timer.GetMilliseconds();

	m_proxyCount = m_tree.GetProxyCount();
	m_nodeCount = m_tree.m_nodeCount;
	m_treeHeight = m_tree.GetHeight();
	m_heapCount = m_tree.m_maxHeapCount;
	m_treeArea = m_tree.GetAreaRatio();

	m_base = 0;
}
