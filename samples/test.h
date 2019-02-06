#pragma once

#include "dynamic-tree/tree.h"

struct Test
{
	virtual const char* GetName() const = 0;
	virtual void Create(dtTreeHeuristic heuristic, bool rotate) = 0;
	virtual void Destroy() = 0;

	dtTree m_tree;
};
