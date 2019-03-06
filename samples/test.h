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
	
	void Create(dtTreeHeuristic heuristic, bool rotate);
	void Allocate(int count);
	void Destroy();
	void Update(Draw& draw, int reinsertIter, int shuffleIter);

	void RebuildTopDownSAH();
	void RebuildTopDownMedian();
	void RebuildBottomUp();

	dtAABB* m_boxes;
	int* m_proxies;
	int m_count;
	dtTree m_tree;
	int m_base;
	float m_buildTime;
};
