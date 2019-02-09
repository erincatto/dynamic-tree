#pragma once

#include "dynamic-tree/tree.h"

struct Draw;

struct Test
{
	virtual const char* GetName() const = 0;
	virtual void Create(dtTreeHeuristic heuristic, bool rotate) = 0;
	virtual void Destroy() = 0;
	virtual void Update(Draw&) {}

	dtTree m_tree;
};
