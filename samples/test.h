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

#include "dynamic-tree/tree.h"

struct Draw;

struct Test
{
	virtual const char* GetCategory() const = 0;
	virtual const char* GetName() const = 0;
	virtual void CreateBoxes() = 0;
	
	void Create(dtInsertionHeuristic heuristic);
	void Allocate(int count);
	void Destroy();
	virtual void Update(Draw& draw, int reinsertIter, int shuffleIter);

	void RebuildTopDownSAH();
	void RebuildTopDownMedian();
	void RebuildBottomUp();

	dtAABB* m_boxes;
	int* m_proxies;
	int m_count;
	dtTree m_tree;
	int m_base;
	float m_buildTime;

	int m_proxyCount = 0;
	int m_nodeCount = 0;
	int m_treeHeight = 0;
	int m_heapCount = 0;
	float m_treeArea = 0.0f;
};

int RegisterTest(Test* test);

static const int s_maxTests = 128;
extern Test* g_tests[s_maxTests];
extern int g_testCount;
